#include "hare/base/fwd-inl.h"
#include "hare/base/io/local.h"
#include "hare/base/io/reactor.h"
#include <hare/base/exception.h>
#include <hare/base/io/socket_op.h>
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

#define close closesocket
#define read _read
#define write _write
#endif

namespace hare {
namespace io {

    namespace detail {

#if HARE__HAVE_EVENTFD
        static auto create_notify_fd() -> util_socket_t
        {
            auto evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            if (evtfd < 0) {
                throw exception("failed to get event fd in ::eventfd.");
            }
            return evtfd;
        }
#else
        static auto create_notify_fd(util_socket_t& _write_fd) -> util_socket_t
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

        class event_notify : public event {
#if HARE__HAVE_EVENTFD
        public:
            event_notify()
                : event(create_notify_fd(),
#else
            util_socket_t write_fd_ {};

        public:
            event_notify()
                : event(create_notify_fd(write_fd_),
#endif
                    std::bind(&event_notify::event_callback,
                        this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                    EVENT_READ | EVENT_PERSIST,
                    0)
            { }

            ~event_notify() override
            {
                ::close(fd());
            }

            void send_notify()
            {
                std::uint64_t one = 1;
                auto write_n = ::write((int)fd(), &one, sizeof(one));
                if (write_n != sizeof(one)) {
                    MSG_ERROR("write[{} B] instead of {} B", write_n, sizeof(one));
                }
            }

            void event_callback(const event::ptr& _event, std::uint8_t _events, const timestamp& _receive_time)
            {
                ignore_unused(_event);
                ignore_unused(_receive_time);
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

        struct total_init {
            total_init()
            {
#if !defined(H_OS_WIN)
                MSG_TRACE("ignore signal[SIGPIPE].");
                auto ret = ::signal(SIGPIPE, SIG_IGN);
                ignore_unused(ret);
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

        auto get_wait_time(const priority_timer& _ptimer) -> std::int32_t
        {
            if (_ptimer.empty()) {
                return POLL_TIME_MICROSECONDS;
            }
            auto time = static_cast<std::int32_t>(_ptimer.top().stamp_.microseconds_since_epoch() - timestamp::now().microseconds_since_epoch());

#if defined(H_OS_WIN)
            return time <= 0 ? static_cast<std::int32_t>(1) : min(time, POLL_TIME_MICROSECONDS);
#else
            return time <= 0 ? static_cast<std::int32_t>(1) : std::min(time, POLL_TIME_MICROSECONDS);
#endif
        }

        void print_active_events(const events_list& _active_events)
        {
            for (const auto& event_elem : _active_events) {
                MSG_TRACE("event[{}] debug info: {}.",
                    event_elem.event_->fd(), event_elem.event_->event_string());
                ignore_unused(event_elem);
            }
        }
    } // namespace detail

    HARE_IMPL_DEFAULT(cycle,
        timestamp reactor_time_ {};
        std::uint64_t tid_ { 0 };
        bool is_running_ { false };
        bool quit_ { false };
        bool event_handling_ { false };
        bool calling_pending_functions_ { false };

        ptr<detail::event_notify> notify_event_ { nullptr };
        ptr<reactor> reactor_ { nullptr };
        ptr<event> current_active_event_ { nullptr };
        event::id event_id_ { 0 };

        mutable std::mutex functions_mutex_ {};
        std::list<task> pending_functions_ {};

        std::uint64_t cycle_index_ { 0 };
    )

    cycle::cycle(REACTOR_TYPE _type)
        : impl_(new cycle_impl)
    {
        static detail::total_init s_total_init {};

        d_ptr(impl_)->tid_ = current_thread::get_tds().tid;
        d_ptr(impl_)->notify_event_.reset(new detail::event_notify());
        d_ptr(impl_)->reactor_.reset(reactor::create_by_type(_type, this));

        if (current_thread::get_tds().cycle != nullptr) {
            MSG_FATAL("another cycle[{}] exists in this thread[{:#x}]",
                (void*)current_thread::get_tds().cycle, current_thread::get_tds().tid);
        } else {
            current_thread::get_tds().cycle = this;
            MSG_TRACE("cycle[{}] is being initialized in thread[{:#x}].", (void*)this, current_thread::get_tds().tid);
        }
    }

    cycle::~cycle()
    {
        // clear thread local data.
        assert_in_cycle_thread();
        d_ptr(impl_)->pending_functions_.clear();
        d_ptr(impl_)->notify_event_.reset();
        current_thread::get_tds().cycle = nullptr;
        delete impl_;
    }

    auto cycle::reactor_return_time() const -> timestamp
    {
        return d_ptr(impl_)->reactor_time_;
    }

    auto cycle::event_handling() const -> bool
    {
        return d_ptr(impl_)->calling_pending_functions_;
    }

    auto cycle::is_running() const -> bool
    {
        return d_ptr(impl_)->is_running_;
    }

#if HARE_DEBUG

    auto cycle::cycle_index() const -> std::uint64_t
    {
        return d_ptr(impl_)->cycle_index_;
    }

#endif

    auto cycle::in_cycle_thread() const -> bool
    {
        return d_ptr(impl_)->tid_ == current_thread::get_tds().tid;
    }

    auto cycle::type() const -> REACTOR_TYPE
    {
        return d_ptr(impl_)->reactor_->type();
    }

    void cycle::loop()
    {
        assert(!d_ptr(impl_)->is_running_);
        d_ptr(impl_)->is_running_ = true;
        d_ptr(impl_)->quit_ = false;
        event_update(d_ptr(impl_)->notify_event_);
        d_ptr(impl_)->notify_event_->tie(d_ptr(impl_)->notify_event_);

        MSG_TRACE("cycle[{}] start running...", (void*)this);

        while (!d_ptr(impl_)->quit_) {
            d_ptr(impl_)->reactor_->active_events_.clear();

            d_ptr(impl_)->reactor_time_ = d_ptr(impl_)->reactor_->poll(get_wait_time(d_ptr(impl_)->reactor_->ptimer_));

#if HARE_DEBUG
            ++d_ptr(impl_)->cycle_index_;
#endif

            print_active_events(d_ptr(impl_)->reactor_->active_events_);

            /// TODO(l1ang70): sort event by priority
            d_ptr(impl_)->event_handling_ = true;

            for (auto& event_elem : d_ptr(impl_)->reactor_->active_events_) {
                d_ptr(impl_)->current_active_event_ = event_elem.event_;
                d_ptr(impl_)->current_active_event_->handle_event(event_elem.revents_, d_ptr(impl_)->reactor_time_);
                if (CHECK_EVENT(d_ptr(impl_)->current_active_event_->events(), EVENT_PERSIST) == 0) {
                    event_remove(d_ptr(impl_)->current_active_event_);
                }
            }
            d_ptr(impl_)->current_active_event_.reset();

            d_ptr(impl_)->event_handling_ = false;

            notify_timer();
            do_pending_functions();
        }

        d_ptr(impl_)->notify_event_->deactivate();
        d_ptr(impl_)->notify_event_->tie(nullptr);
        d_ptr(impl_)->is_running_ = false;

        d_ptr(impl_)->reactor_->active_events_.clear();
        for (const auto& event : d_ptr(impl_)->reactor_->events_) {
            event.second->reset();
        }
        d_ptr(impl_)->reactor_->events_.clear();
        priority_timer().swap(d_ptr(impl_)->reactor_->ptimer_);

        MSG_TRACE("cycle[{}] stop running...", (void*)this);
    }

    void cycle::exit()
    {
        d_ptr(impl_)->quit_ = true;

        /**
         * @brief There is a chance that loop() just executes while(!quit_) and exits,
         *   then Cycle destructs, then we are accessing an invalid object.
         */
        if (!in_cycle_thread()) {
            notify();
        }
    }

    void cycle::run_in_cycle(task _task)
    {
        if (in_cycle_thread()) {
            _task();
        } else {
            queue_in_cycle(std::move(_task));
        }
    }

    void cycle::queue_in_cycle(task _task)
    {
        {
            std::lock_guard<std::mutex> guard(d_ptr(impl_)->functions_mutex_);
            d_ptr(impl_)->pending_functions_.push_back(std::move(_task));
        }

        if (!in_cycle_thread()) {
            notify();
        }
    }

    auto cycle::queue_size() const -> std::size_t
    {
        std::lock_guard<std::mutex> guard(d_ptr(impl_)->functions_mutex_);
        return d_ptr(impl_)->pending_functions_.size();
    }

    void cycle::event_update(const hare::ptr<event>& _event)
    {
        if (_event->owner_cycle() != this && _event->event_id() != -1) {
            MSG_ERROR("cannot add event from other cycle[{}]", (void*)_event->owner_cycle());
            return;
        }

        run_in_cycle(std::bind([=](const wptr<event>& wevent) {
            auto sevent = wevent.lock();
            if (!sevent) {
                return;
            }

            bool ret = true;

            if (sevent->fd() >= 0) {
                ret = d_ptr(impl_)->reactor_->event_update(sevent);
            }

            if (!ret) {
                return;
            }

            if (sevent->event_id() == -1) {
                sevent->active(this, d_ptr(impl_)->event_id_++);

                assert(d_ptr(impl_)->reactor_->events_.find(sevent->event_id()) == d_ptr(impl_)->reactor_->events_.end());
                d_ptr(impl_)->reactor_->events_.insert(std::make_pair(sevent->event_id(), sevent));

                if (sevent->fd() >= 0) {
                    d_ptr(impl_)->reactor_->inverse_map_.insert(std::make_pair(sevent->fd(), sevent->event_id()));
                }
            }

            if (CHECK_EVENT(sevent->events(), EVENT_TIMEOUT) != 0) {
                d_ptr(impl_)->reactor_->ptimer_.emplace(sevent->event_id(),
                    timestamp(timestamp::now().microseconds_since_epoch() + sevent->timeval()));
            }
        },
            _event));
    }

    void cycle::event_remove(const hare::ptr<event>& _event)
    {
        if (!_event) {
            return;
        }

        if (_event->owner_cycle() != this || _event->event_id() == -1) {
            MSG_ERROR("the event is not part of the cycle[{}]", (void*)_event->owner_cycle());
            return;
        }

        run_in_cycle(std::bind([=](const wptr<event>& wevent) {
            auto sevent = wevent.lock();
            if (!sevent) {
                MSG_ERROR("event is empty before it was released.");
                return;
            }
            auto event_id = sevent->event_id();
            auto socket = sevent->fd();

            auto iter = d_ptr(impl_)->reactor_->events_.find(event_id);
            if (iter == d_ptr(impl_)->reactor_->events_.end()) {
                MSG_ERROR("cannot find event in cycle[{}]", (void*)this);
            } else {
                assert(iter->second == sevent);

                sevent->reset();

                if (socket >= 0) {
                    d_ptr(impl_)->reactor_->event_remove(sevent);
                    d_ptr(impl_)->reactor_->inverse_map_.erase(socket);
                }

                d_ptr(impl_)->reactor_->events_.erase(iter);
            }
        },
            _event));
    }

    auto cycle::event_check(const hare::ptr<event>& _event) -> bool
    {
        if (!_event || _event->event_id() < 0) {
            return false;
        }
        assert_in_cycle_thread();
        return d_ptr(impl_)->reactor_->events_.find(_event->event_id()) != d_ptr(impl_)->reactor_->events_.end();
    }

    void cycle::notify()
    {
        d_ptr(impl_)->notify_event_->send_notify();
    }

    void cycle::abort_not_cycle_thread()
    {
        MSG_FATAL("cycle[{}] was created in thread[{:#x}], current thread is: {:#x}",
            (void*)this, d_ptr(impl_)->tid_, current_thread::get_tds().tid);
    }

    void cycle::do_pending_functions()
    {
        std::list<task> functions {};
        d_ptr(impl_)->calling_pending_functions_ = true;

        {
            std::lock_guard<std::mutex> guard(d_ptr(impl_)->functions_mutex_);
            functions.swap(d_ptr(impl_)->pending_functions_);
        }

        for (const auto& function : functions) {
            function();
        }

        d_ptr(impl_)->calling_pending_functions_ = false;
    }

    void cycle::notify_timer()
    {
        auto& priority_event = d_ptr(impl_)->reactor_->ptimer_;
        auto& events = d_ptr(impl_)->reactor_->events_;
        const auto revent = EVENT_TIMEOUT;
        auto now = timestamp::now();

        while (!priority_event.empty()) {
            auto top = priority_event.top();
            if (d_ptr(impl_)->reactor_time_ < top.stamp_) {
                break;
            }
            auto iter = events.find(top.id_);
            if (iter != events.end()) {
                auto& event = iter->second;
                if (event) {
                    MSG_TRACE("event[{}] trigged.", (void*)iter->second.get());
                    event->handle_event(revent, now);
                    if ((event->events() & EVENT_PERSIST) != 0) {
                        priority_event.emplace(top.id_, timestamp(d_ptr(impl_)->reactor_time_.microseconds_since_epoch() + event->timeval()));
                    } else {
                        event_remove(event);
                    }
                } else {
                    events.erase(iter);
                }
            } else {
                MSG_TRACE("event[{}] deleted.", top.id_);
            }
            priority_event.pop();
        }
    }

} // namespace io
} // namespace hare
