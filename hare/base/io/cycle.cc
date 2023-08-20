#include "base/fwd-inl.h"
#include "base/io/local.h"
#include "base/io/reactor.h"
#include "base/io/socket_op-inl.h"
#include <hare/base/exception.h>
#include <hare/base/io/timer.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/hare-config.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <list>
#include <mutex>
#include <utility>

#if HARE__HAVE_EVENTFD
#include <sys/eventfd.h>
#endif

#ifdef H_OS_UNIX
#include <sys/socket.h>
#elif defined(H_OS_WIN)
#include <WinSock2.h>
#define read _read
#define write _write
#endif

namespace hare {
namespace io {

    namespace detail {

#if HARE__HAVE_EVENTFD
        static auto CreateNotifier() -> util_socket_t
        {
            auto evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            if (evtfd < 0) {
                HARE_INTERNAL_FATAL("failed to get event fd in ::eventfd.");
            }
            return evtfd;
        }
#else
        static auto CreateNotifier(util_socket_t& _write_fd) -> util_socket_t
        {
            util_socket_t fd[2];
            auto ret = socket_op::socketpair(AF_INET, SOCK_STREAM, 0, fd);
            if (ret < 0) {
                HARE_INTERNAL_FATAL("fail to create socketpair for notify_fd.");
            }
            _write_fd = fd[0];
            return fd[1];
        }
#endif

        class EventNotify : public Event {
#if HARE__HAVE_EVENTFD
        public:
            EventNotify()
                : Event(CreateNotifier(),
#else
            util_socket_t write_fd_ {};

        public:
            event_notify()
                : event(CreateNotifier(write_fd_),
#endif
                    std::bind(&EventNotify::EventCallback,
                        this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                    EVENT_READ | EVENT_PERSIST,
                    0)
            {
            }

            ~EventNotify() override
            {
                socket_op::Close(fd());
            }

            void SendNotify()
            {
                std::uint64_t one = 1;
                auto write_n = ::write((int)fd(), &one, sizeof(one));
                if (write_n != sizeof(one)) {
                    HARE_INTERNAL_ERROR("write[{} B] instead of {} B", write_n, sizeof(one));
                }
            }

            void EventCallback(const Ptr<Event>& _event, std::uint8_t _events, const Timestamp& _receive_time)
            {
                IgnoreUnused(_event, _receive_time);
                assert(_event == this->shared_from_this());

                if (CHECK_EVENT(_events, EVENT_READ) != 0) {
                    std::uint64_t one = 0;
                    auto read_n = ::read((int)fd(), &one, sizeof(one));
                    if (read_n != sizeof(one) && one != static_cast<std::uint64_t>(1)) {
                        HARE_INTERNAL_ERROR("read notify[{} B] instead of {} B", read_n, sizeof(one));
                    }
                } else {
                    HARE_INTERNAL_FATAL("an error occurred while accepting notify in fd[{}]", fd());
                }
            }
        };

        struct GlobalInit {
            GlobalInit()
            {
#if !defined(H_OS_WIN)
                HARE_INTERNAL_TRACE("ignore signal[SIGPIPE].");
                auto ret = ::signal(SIGPIPE, SIG_IGN);
                IgnoreUnused(ret);
            }
#else
                WSADATA wsa_data {};
                if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) == SOCKET_ERROR) {
                    HARE_INTERNAL_FATAL("fail to ::WSAStartup.");
                }
                HARE_INTERNAL_TRACE("::WSAStartup success.");
            }

            ~total_init()
            {
                ignore_unused(::WSACleanup());
                HARE_INTERNAL_TRACE("::WSACleanup success.");
            }
#endif
        };

        static auto GetWaitTime(const PriorityTimer& _ptimer) -> std::int32_t
        {
            if (_ptimer.empty()) {
                return POLL_TIME_MICROSECONDS;
            }
            auto time = static_cast<std::int32_t>(_ptimer.top().stamp.microseconds_since_epoch() - Timestamp::Now().microseconds_since_epoch());

            return time <= 0 ? 1 : MIN(time, POLL_TIME_MICROSECONDS);
        }

        static void PrintActiveEvents(const EventsList& _active_events)
        {
            for (const auto& event_elem : _active_events) {
                HARE_INTERNAL_TRACE("event[{}] debug info: {}.",
                    event_elem.event->fd(), event_elem.event->EventToString());
                IgnoreUnused(event_elem);
            }
        }
    } // namespace detail

    HARE_IMPL_DEFAULT(
        Cycle,
        Timestamp reactor_time {};
        std::uint64_t tid { 0 };
        bool is_running { false };
        bool quit { false };
        bool event_handling { false };
        bool calling_pending_functions { false };

        Ptr<detail::EventNotify> notify_event { nullptr };
        Ptr<Reactor> reactor { nullptr };
        Ptr<Event> current_active_event { nullptr };
        std::atomic<Event::Id> event_id { 0 };

        mutable std::mutex functions_mutex {};
        std::list<Task> pending_functions {};

        std::uint64_t cycle_index { 0 };
    )

    Cycle::Cycle(REACTOR_TYPE _type)
        : impl_(new CycleImpl)
    {
        static detail::GlobalInit s_total_init {};

        IMPL->tid = current_thread::ThreadData().tid;
        IMPL->notify_event.reset(new detail::EventNotify());
        IMPL->reactor.reset(CHECK_NULL(Reactor::CreateByType(_type, this)));

        if (current_thread::ThreadData().cycle != nullptr) {
            HARE_INTERNAL_FATAL("another cycle[{}] exists in this thread[{:#x}]",
                (void*)current_thread::ThreadData().cycle, current_thread::ThreadData().tid);
        } else {
            current_thread::ThreadData().cycle = this;
            HARE_INTERNAL_TRACE("cycle[{}] is being initialized in thread[{:#x}].", (void*)this, current_thread::ThreadData().tid);
        }
    }

    Cycle::~Cycle()
    {
        // clear thread local data.
        AssertInCycleThread();
        IMPL->pending_functions.clear();
        IMPL->notify_event.reset();
        current_thread::ThreadData().cycle = nullptr;
        delete impl_;
    }

    auto Cycle::ReactorReturnTime() const -> Timestamp
    {
        return IMPL->reactor_time;
    }

    auto Cycle::EventHandling() const -> bool
    {
        return IMPL->calling_pending_functions;
    }

    auto Cycle::is_running() const -> bool
    {
        return IMPL->is_running;
    }

    auto Cycle::type() const -> REACTOR_TYPE
    {
        return IMPL->reactor->type();
    }

#if HARE_DEBUG

    auto Cycle::cycle_index() const -> std::uint64_t
    {
        return IMPL->cycle_index;
    }

#endif

    auto Cycle::InCycleThread() const -> bool
    {
        return IMPL->tid == current_thread::ThreadData().tid;
    }

    void Cycle::Exec()
    {
        assert(!IMPL->is_running);
        IMPL->is_running = true;
        IMPL->quit = false;
        EventUpdate(IMPL->notify_event);
        IMPL->notify_event->Tie(IMPL->notify_event);

        HARE_INTERNAL_TRACE("cycle[{}] start running...", (void*)this);

        while (!IMPL->quit) {
            IMPL->reactor->active_events_.clear();

            IMPL->reactor_time = IMPL->reactor->Poll(GetWaitTime(IMPL->reactor->ptimer_));

#if HARE_DEBUG
            ++IMPL->cycle_index;
#endif

            PrintActiveEvents(IMPL->reactor->active_events_);

            /// TODO(l1ang70): sort event by priority
            IMPL->event_handling = true;

            for (auto& event_elem : IMPL->reactor->active_events_) {
                IMPL->current_active_event = event_elem.event;
                IMPL->current_active_event->HandleEvent(event_elem.revents, IMPL->reactor_time);
                if (CHECK_EVENT(IMPL->current_active_event->events(), EVENT_PERSIST) == 0) {
                    EventRemove(IMPL->current_active_event);
                }
            }
            IMPL->current_active_event.reset();

            IMPL->event_handling = false;

            NotifyTimer();
            DoPendingFunctions();
        }

        IMPL->notify_event->Deactivate();
        IMPL->notify_event->Tie(nullptr);
        IMPL->is_running = false;

        IMPL->reactor->active_events_.clear();
        for (const auto& event : IMPL->reactor->events_) {
            event.second->Reset();
        }
        IMPL->reactor->events_.clear();
        PriorityTimer().swap(IMPL->reactor->ptimer_);

        HARE_INTERNAL_TRACE("cycle[{}] stop running...", (void*)this);
    }

    void Cycle::Exit()
    {
        IMPL->quit = true;

        /**
         * @brief There is a chance that loop() just executes while(!quit_) and exits,
         *   then Cycle destructs, then we are accessing an invalid object.
         */
        if (!InCycleThread()) {
            Notify();
        }
    }

    void Cycle::RunInCycle(Task _task)
    {
        if (InCycleThread()) {
            _task();
        } else {
            QueueInCycle(std::move(_task));
        }
    }

    void Cycle::QueueInCycle(Task _task)
    {
        {
            std::lock_guard<std::mutex> guard(IMPL->functions_mutex);
            IMPL->pending_functions.push_back(std::move(_task));
        }

        if (!InCycleThread()) {
            Notify();
        }
    }

    auto Cycle::QueueSize() const -> std::size_t
    {
        std::lock_guard<std::mutex> guard(IMPL->functions_mutex);
        return IMPL->pending_functions.size();
    }

    auto Cycle::RunAfter(const Task& _task, std::int64_t _delay) -> Event::Id
    {
        if (!is_running()) {
            return -1;
        }
        auto timer = std::make_shared<Timer>(_task, _delay);
        auto id = IMPL->event_id.fetch_add(1);

        RunInCycle([=] {
            assert(timer->id() == -1);

            timer->Active(this, id);
            assert(IMPL->reactor->events_.find(timer->id()) == IMPL->reactor->events_.end());
            IMPL->reactor->events_.insert(std::make_pair(timer->id(), timer));

            assert(CHECK_EVENT(timer->events(), EVENT_TIMEOUT) != 0);
            IMPL->reactor->ptimer_.emplace(timer->id(),
                Timestamp(Timestamp::Now().microseconds_since_epoch() + timer->timeval()));

        });

        return id;
    }

    auto Cycle::RunEvery(const Task& _task, std::int64_t _delay) -> Event::Id
    {
        if (!is_running()) {
            return -1;
        }
        
        auto timer = std::make_shared<Timer>(_task, _delay, true);
        auto id = IMPL->event_id.fetch_add(1);

        RunInCycle([=] {
            assert(timer->id() == -1);

            timer->Active(this, id);
            assert(IMPL->reactor->events_.find(timer->id()) == IMPL->reactor->events_.end());
            IMPL->reactor->events_.insert(std::make_pair(timer->id(), timer));

            assert(CHECK_EVENT(timer->events(), EVENT_TIMEOUT) != 0);
            IMPL->reactor->ptimer_.emplace(timer->id(),
                Timestamp(Timestamp::Now().microseconds_since_epoch() + timer->timeval()));
        });

        return id;
    }

    void Cycle::Cancel(Event::Id _event_id)
    {
        if (!is_running()) {
            return;
        }

        util::CountDownLatch cdl { 1 };
        
        RunInCycle([&] {
            auto iter = IMPL->reactor->events_.find(_event_id);
            if (iter != IMPL->reactor->events_.end()) {
                auto event = iter->second;
                if (event->fd() < 0) {
                    event->Reset();
                    IMPL->reactor->events_.erase(iter);
                } else {
                    HARE_INTERNAL_ERROR("cannot \'Cancel\' an event with non-zero file descriptors.");
                }
            } else {
                HARE_INTERNAL_TRACE("event[{}] already finished/cancelled!", _event_id);
            }
            cdl.CountDown();
        });

        if (!InCycleThread()) {
            cdl.Await();
        }
    }

    void Cycle::EventUpdate(const hare::Ptr<Event>& _event)
    {
        if (_event->cycle() != this && _event->id() != -1) {
            HARE_INTERNAL_ERROR("cannot add event from other cycle[{}].", (void*)_event->cycle());
            return;
        }

        RunInCycle(std::bind([=](const WPtr<Event>& wevent) {
            auto sevent = wevent.lock();
            if (!sevent) {
                return;
            }

            bool ret = true;

            if (sevent->fd() >= 0) {
                ret = IMPL->reactor->EventUpdate(sevent);
            }

            if (!ret) {
                return;
            }

            if (sevent->id() == -1) {
                sevent->Active(this, IMPL->event_id.fetch_add(1));

                assert(IMPL->reactor->events_.find(sevent->id()) == IMPL->reactor->events_.end());
                IMPL->reactor->events_.insert(std::make_pair(sevent->id(), sevent));

                if (sevent->fd() >= 0) {
                    IMPL->reactor->inverse_map_.insert(std::make_pair(sevent->fd(), sevent->id()));
                }
            }

            if (CHECK_EVENT(sevent->events(), EVENT_TIMEOUT) != 0) {
                IMPL->reactor->ptimer_.emplace(sevent->id(),
                    Timestamp(Timestamp::Now().microseconds_since_epoch() + sevent->timeval()));
            }
        },
            _event));
    }

    void Cycle::EventRemove(const hare::Ptr<Event>& _event)
    {
        if (!_event) {
            return;
        }

        if (_event->cycle() != this || _event->id() == -1) {
            HARE_INTERNAL_ERROR("the event is not part of the cycle[{}].", (void*)_event->cycle());
            return;
        }

        RunInCycle(std::bind([=](const WPtr<Event>& wevent) {
            auto sevent = wevent.lock();
            if (!sevent) {
                HARE_INTERNAL_ERROR("event is empty before it was released.");
                return;
            }
            auto event_id = sevent->id();
            auto socket = sevent->fd();

            auto iter = IMPL->reactor->events_.find(event_id);
            if (iter == IMPL->reactor->events_.end()) {
                HARE_INTERNAL_ERROR("cannot find event in cycle[{}]", (void*)this);
            } else {
                assert(iter->second == sevent);

                sevent->Reset();

                if (socket >= 0) {
                    IMPL->reactor->EventRemove(sevent);
                    IMPL->reactor->inverse_map_.erase(socket);
                }

                IMPL->reactor->events_.erase(iter);
            }
        },
            _event));
    }

    auto Cycle::EventCheck(const hare::Ptr<Event>& _event) -> bool
    {
        if (!_event || _event->id() < 0) {
            return false;
        }
        AssertInCycleThread();
        return IMPL->reactor->events_.find(_event->id()) != IMPL->reactor->events_.end();
    }

    void Cycle::Notify()
    {
        IMPL->notify_event->SendNotify();
    }

    void Cycle::AbortNotCycleThread()
    {
        HARE_INTERNAL_FATAL("cycle[{}] was created in thread[{:#x}], current thread is: {:#x}",
            (void*)this, IMPL->tid, current_thread::ThreadData().tid);
    }

    void Cycle::DoPendingFunctions()
    {
        std::list<Task> functions {};
        IMPL->calling_pending_functions = true;

        {
            std::lock_guard<std::mutex> guard(IMPL->functions_mutex);
            functions.swap(IMPL->pending_functions);
        }

        for (const auto& function : functions) {
            function();
        }

        IMPL->calling_pending_functions = false;
    }

    void Cycle::NotifyTimer()
    {
        auto& priority_event = IMPL->reactor->ptimer_;
        auto& events = IMPL->reactor->events_;
        const auto revent = EVENT_TIMEOUT;
        auto now = Timestamp::Now();

        while (!priority_event.empty()) {
            auto top = priority_event.top();
            if (IMPL->reactor_time < top.stamp) {
                break;
            }
            auto iter = events.find(top.id);
            if (iter != events.end()) {
                auto& event = iter->second;
                if (event) {
                    HARE_INTERNAL_TRACE("event[{}] trigged.", (void*)iter->second.get());
                    event->HandleEvent(revent, now);
                    if ((event->events() & EVENT_PERSIST) != 0) {
                        priority_event.emplace(top.id, Timestamp(IMPL->reactor_time.microseconds_since_epoch() + event->timeval()));
                    } else {
                        EventRemove(event);
                    }
                } else {
                    events.erase(iter);
                }
            } else {
                HARE_INTERNAL_TRACE("event[{}] deleted.", top.id);
            }
            priority_event.pop();
        }
    }

} // namespace io
} // namespace hare
