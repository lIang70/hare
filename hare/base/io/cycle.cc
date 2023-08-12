#include "hare/base/fwd-inl.h"
#include "hare/base/io/local.h"
#include "hare/base/io/reactor.h"
#include "hare/base/io/socket_op-inl.h"
#include <hare/base/exception.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/hare-config.h>

#include <algorithm>
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
                throw Exception("failed to get event fd in ::eventfd.");
            }
            return evtfd;
        }
#else
        static auto CreateNotifier(util_socket_t& _write_fd) -> util_socket_t
        {
            util_socket_t fd[2];
            auto ret = socket_op::socketpair(AF_INET, SOCK_STREAM, 0, fd);
            if (ret < 0) {
                MSG_FATAL("fail to create socketpair for notify_fd.");
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
            { }

            ~EventNotify() override
            {
                socket_op::Close(fd());
            }

            void SendNotify()
            {
                std::uint64_t one = 1;
                auto write_n = ::write((int)fd(), &one, sizeof(one));
                if (write_n != sizeof(one)) {
                    MSG_ERROR("write[{} B] instead of {} B", write_n, sizeof(one));
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
                        MSG_ERROR("read notify[{} B] instead of {} B", read_n, sizeof(one));
                    }
                } else {
                    MSG_FATAL("an error occurred while accepting notify in fd[{}]", fd());
                }
            }
        };

        struct GlobalInit {
            GlobalInit()
            {
#if !defined(H_OS_WIN)
                MSG_TRACE("ignore signal[SIGPIPE].");
                auto ret = ::signal(SIGPIPE, SIG_IGN);
                IgnoreUnused(ret);
            }
#else
                WSADATA wsa_data {};
                if (::WSAStartup(MAKEWORD(2, 2), &wsa_data) == SOCKET_ERROR) {
                    MSG_FATAL("fail to ::WSAStartup.");
                }
                MSG_TRACE("::WSAStartup success.");
            }

            ~total_init()
            {
                ignore_unused(::WSACleanup());
                MSG_TRACE("::WSACleanup success.");
            }
#endif
        };

        auto GetWaitTime(const PriorityTimer& _ptimer) -> std::int32_t
        {
            if (_ptimer.empty()) {
                return POLL_TIME_MICROSECONDS;
            }
            auto time = static_cast<std::int32_t>(_ptimer.top().stamp.microseconds_since_epoch() - Timestamp::Now().microseconds_since_epoch());

            return time <= 0 ? 1 : MIN(time, POLL_TIME_MICROSECONDS);
        }

        void PrintActiveEvents(const EventsList& _active_events)
        {
            for (const auto& event_elem : _active_events) {
                MSG_TRACE("event[{}] debug info: {}.",
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
        Event::Id event_id { 0 };

        mutable std::mutex functions_mutex {};
        std::list<Task> pending_functions {};

        std::uint64_t cycle_index { 0 };
    )

    Cycle::Cycle(REACTOR_TYPE _type)
        : impl_(new CycleImpl)
    {
        static detail::GlobalInit s_total_init {};

        d_ptr(impl_)->tid = current_thread::ThreadData().tid;
        d_ptr(impl_)->notify_event.reset(new detail::EventNotify());
        d_ptr(impl_)->reactor.reset(Reactor::CreateByType(_type, this));

        if (current_thread::ThreadData().cycle != nullptr) {
            MSG_FATAL("another cycle[{}] exists in this thread[{:#x}]",
                (void*)current_thread::ThreadData().cycle, current_thread::ThreadData().tid);
        } else {
            current_thread::ThreadData().cycle = this;
            MSG_TRACE("cycle[{}] is being initialized in thread[{:#x}].", (void*)this, current_thread::ThreadData().tid);
        }
    }

    Cycle::~Cycle()
    {
        // clear thread local data.
        AssertInCycleThread();
        d_ptr(impl_)->pending_functions.clear();
        d_ptr(impl_)->notify_event.reset();
        current_thread::ThreadData().cycle = nullptr;
        delete impl_;
    }

    auto Cycle::ReactorReturnTime() const -> Timestamp
    {
        return d_ptr(impl_)->reactor_time;
    }

    auto Cycle::event_handling() const -> bool
    {
        return d_ptr(impl_)->calling_pending_functions;
    }

    auto Cycle::is_running() const -> bool
    {
        return d_ptr(impl_)->is_running;
    }

    auto Cycle::type() const -> REACTOR_TYPE
    {
        return d_ptr(impl_)->reactor->type();
    }

#if HARE_DEBUG

    auto Cycle::cycle_index() const -> std::uint64_t
    {
        return d_ptr(impl_)->cycle_index;
    }

#endif

    auto Cycle::InCycleThread() const -> bool
    {
        return d_ptr(impl_)->tid == current_thread::ThreadData().tid;
    }

    void Cycle::Exec()
    {
        assert(!d_ptr(impl_)->is_running);
        d_ptr(impl_)->is_running = true;
        d_ptr(impl_)->quit = false;
        EventUpdate(d_ptr(impl_)->notify_event);
        d_ptr(impl_)->notify_event->Tie(d_ptr(impl_)->notify_event);

        MSG_TRACE("cycle[{}] start running...", (void*)this);

        while (!d_ptr(impl_)->quit) {
            d_ptr(impl_)->reactor->active_events_.clear();

            d_ptr(impl_)->reactor_time = d_ptr(impl_)->reactor->Poll(GetWaitTime(d_ptr(impl_)->reactor->ptimer_));

#if HARE_DEBUG
            ++d_ptr(impl_)->cycle_index;
#endif

            PrintActiveEvents(d_ptr(impl_)->reactor->active_events_);

            /// TODO(l1ang70): sort event by priority
            d_ptr(impl_)->event_handling = true;

            for (auto& event_elem : d_ptr(impl_)->reactor->active_events_) {
                d_ptr(impl_)->current_active_event = event_elem.event;
                d_ptr(impl_)->current_active_event->HandleEvent(event_elem.revents, d_ptr(impl_)->reactor_time);
                if (CHECK_EVENT(d_ptr(impl_)->current_active_event->events(), EVENT_PERSIST) == 0) {
                    EventRemove(d_ptr(impl_)->current_active_event);
                }
            }
            d_ptr(impl_)->current_active_event.reset();

            d_ptr(impl_)->event_handling = false;

            NotifyTimer();
            DoPendingFunctions();
        }

        d_ptr(impl_)->notify_event->Deactivate();
        d_ptr(impl_)->notify_event->Tie(nullptr);
        d_ptr(impl_)->is_running = false;

        d_ptr(impl_)->reactor->active_events_.clear();
        for (const auto& event : d_ptr(impl_)->reactor->events_) {
            event.second->Reset();
        }
        d_ptr(impl_)->reactor->events_.clear();
        PriorityTimer().swap(d_ptr(impl_)->reactor->ptimer_);

        MSG_TRACE("cycle[{}] stop running...", (void*)this);
    }

    void Cycle::Exit()
    {
        d_ptr(impl_)->quit = true;

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
            std::lock_guard<std::mutex> guard(d_ptr(impl_)->functions_mutex);
            d_ptr(impl_)->pending_functions.push_back(std::move(_task));
        }

        if (!InCycleThread()) {
            Notify();
        }
    }

    auto Cycle::queue_size() const -> std::size_t
    {
        std::lock_guard<std::mutex> guard(d_ptr(impl_)->functions_mutex);
        return d_ptr(impl_)->pending_functions.size();
    }

    void Cycle::EventUpdate(const hare::Ptr<Event>& _event)
    {
        if (_event->cycle() != this && _event->id() != -1) {
            MSG_ERROR("cannot add event from other cycle[{}]", (void*)_event->cycle());
            return;
        }

        RunInCycle(std::bind([=](const WPtr<Event>& wevent) {
            auto sevent = wevent.lock();
            if (!sevent) {
                return;
            }

            bool ret = true;

            if (sevent->fd() >= 0) {
                ret = d_ptr(impl_)->reactor->EventUpdate(sevent);
            }

            if (!ret) {
                return;
            }

            if (sevent->id() == -1) {
                sevent->Active(this, d_ptr(impl_)->event_id++);

                assert(d_ptr(impl_)->reactor->events_.find(sevent->id()) == d_ptr(impl_)->reactor->events_.end());
                d_ptr(impl_)->reactor->events_.insert(std::make_pair(sevent->id(), sevent));

                if (sevent->fd() >= 0) {
                    d_ptr(impl_)->reactor->inverse_map_.insert(std::make_pair(sevent->fd(), sevent->id()));
                }
            }

            if (CHECK_EVENT(sevent->events(), EVENT_TIMEOUT) != 0) {
                d_ptr(impl_)->reactor->ptimer_.emplace(sevent->id(),
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
            MSG_ERROR("the event is not part of the cycle[{}]", (void*)_event->cycle());
            return;
        }

        RunInCycle(std::bind([=](const WPtr<Event>& wevent) {
            auto sevent = wevent.lock();
            if (!sevent) {
                MSG_ERROR("event is empty before it was released.");
                return;
            }
            auto event_id = sevent->id();
            auto socket = sevent->fd();

            auto iter = d_ptr(impl_)->reactor->events_.find(event_id);
            if (iter == d_ptr(impl_)->reactor->events_.end()) {
                MSG_ERROR("cannot find event in cycle[{}]", (void*)this);
            } else {
                assert(iter->second == sevent);

                sevent->Reset();

                if (socket >= 0) {
                    d_ptr(impl_)->reactor->EventRemove(sevent);
                    d_ptr(impl_)->reactor->inverse_map_.erase(socket);
                }

                d_ptr(impl_)->reactor->events_.erase(iter);
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
        return d_ptr(impl_)->reactor->events_.find(_event->id()) != d_ptr(impl_)->reactor->events_.end();
    }

    void Cycle::Notify()
    {
        d_ptr(impl_)->notify_event->SendNotify();
    }

    void Cycle::AbortNotCycleThread()
    {
        MSG_FATAL("cycle[{}] was created in thread[{:#x}], current thread is: {:#x}",
            (void*)this, d_ptr(impl_)->tid, current_thread::ThreadData().tid);
    }

    void Cycle::DoPendingFunctions()
    {
        std::list<Task> functions {};
        d_ptr(impl_)->calling_pending_functions = true;

        {
            std::lock_guard<std::mutex> guard(d_ptr(impl_)->functions_mutex);
            functions.swap(d_ptr(impl_)->pending_functions);
        }

        for (const auto& function : functions) {
            function();
        }

        d_ptr(impl_)->calling_pending_functions = false;
    }

    void Cycle::NotifyTimer()
    {
        auto& priority_event = d_ptr(impl_)->reactor->ptimer_;
        auto& events = d_ptr(impl_)->reactor->events_;
        const auto revent = EVENT_TIMEOUT;
        auto now = Timestamp::Now();

        while (!priority_event.empty()) {
            auto top = priority_event.top();
            if (d_ptr(impl_)->reactor_time < top.stamp) {
                break;
            }
            auto iter = events.find(top.id);
            if (iter != events.end()) {
                auto& event = iter->second;
                if (event) {
                    MSG_TRACE("event[{}] trigged.", (void*)iter->second.get());
                    event->HandleEvent(revent, now);
                    if ((event->events() & EVENT_PERSIST) != 0) {
                        priority_event.emplace(top.id, Timestamp(d_ptr(impl_)->reactor_time.microseconds_since_epoch() + event->timeval()));
                    } else {
                        EventRemove(event);
                    }
                } else {
                    events.erase(iter);
                }
            } else {
                MSG_TRACE("event[{}] deleted.", top.id);
            }
            priority_event.pop();
        }
    }

} // namespace io
} // namespace hare
