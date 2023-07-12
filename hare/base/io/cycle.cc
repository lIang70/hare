#include "hare/base/io/local.h"
#include "hare/base/io/reactor.h"
#include <hare/base/exception.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/hare-config.h>

#include <algorithm>
#include <csignal>
#include <utility>

#if HARE__HAVE_EVENTFD
#include <sys/eventfd.h>
#endif

#ifdef H_OS_UNIX
#include <sys/socket.h>
#endif

namespace hare {
namespace io {

    namespace detail {

        static auto create_notify_fd() -> util_socket_t
        {
#if HARE__HAVE_EVENTFD
            auto evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            if (evtfd < 0) {
                throw exception("failed to get event fd in ::eventfd.");
            }
            return evtfd;
#else
#error "Do not have ::eventfd()."
#endif
        }

        class event_notify : public event {
        public:
            explicit event_notify()
                : event(create_notify_fd(),
                    std::bind(&event_notify::event_callback,
                        this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                    EVENT_READ | EVENT_PERSIST,
                    0)
            {
            }

            ~event_notify() override
            {
                ::close(fd());
            }

            void send_notify()
            {
                std::uint64_t one = 1;
                auto write_n = ::write(fd(), &one, sizeof(one));
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
                    auto read_n = ::read(fd(), &one, sizeof(one));
                    if (read_n != sizeof(one) && one != static_cast<std::uint64_t>(1)) {
                        MSG_ERROR("read notify[{} B] instead of {} B", read_n, sizeof(one));
                    }
                } else {
                    MSG_FATAL("an error occurred while accepting notify in fd[{}]", fd());
                }
            }
        };

        struct ignore_sigpipe {
            ignore_sigpipe()
            {
                MSG_TRACE("ignore signal[SIGPIPE].");
                auto ret = ::signal(SIGPIPE, SIG_IGN);
                ignore_unused(ret);
            }
        };

        auto get_wait_time(const priority_timer& _ptimer) -> std::int32_t
        {
            if (_ptimer.empty()) {
                return POLL_TIME_MICROSECONDS;
            }
            auto time = static_cast<std::int32_t>(_ptimer.top().stamp_.microseconds_since_epoch() - timestamp::now().microseconds_since_epoch());
            return time <= 0 ? static_cast<std::int32_t>(1) : std::min(time, POLL_TIME_MICROSECONDS);
        }

        void print_active_events(const events_list& _active_events)
        {
            for (const auto& event_elem : _active_events) {
                MSG_TRACE("event[{}] debug info: {}.",
                    event_elem.event_->fd(), event_elem.event_->event_string());
            }
        }
    } // namespace detail

    cycle::cycle(REACTOR_TYPE _type)
        : tid_(current_thread::get_tds().tid)
        , notify_event_(new detail::event_notify())
        , reactor_(reactor::create_by_type(_type, this))
    {
        // Ignore signal[pipe]
        static detail::ignore_sigpipe s_ignore_sigpipe {};

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
        pending_functions_.clear();
        notify_event_.reset();
        current_thread::get_tds().cycle = nullptr;
    }

    auto cycle::in_cycle_thread() const -> bool
    {
        return tid_ == current_thread::get_tds().tid;
    }

    auto cycle::type() const -> REACTOR_TYPE
    {
        return reactor_->type();
    }

    void cycle::loop()
    {
        assert(!is_running_);
        is_running_ = true;
        quit_ = false;
        event_update(notify_event_);
        notify_event_->tie(notify_event_);

        MSG_TRACE("cycle[{}] start running...", (void*)this);

        while (!quit_) {
            reactor_->active_events_.clear();

            reactor_time_ = reactor_->poll(get_wait_time(reactor_->ptimer_));

#if HARE_DEBUG
            ++cycle_index_;
#endif

            print_active_events(reactor_->active_events_);

            /// TODO(l1ang): sort event by priority
            event_handling_ = true;

            for (auto& event_elem : reactor_->active_events_) {
                current_active_event_ = event_elem.event_;
                current_active_event_->handle_event(event_elem.revents_, reactor_time_);
                if (CHECK_EVENT(current_active_event_->events(), EVENT_PERSIST) == 0) {
                    event_remove(current_active_event_);
                }
            }
            current_active_event_.reset();

            event_handling_ = false;

            notify_timer();
            do_pending_functions();
        }

        notify_event_->deactivate();
        notify_event_->tie(nullptr);
        is_running_ = false;

        reactor_->active_events_.clear();
        for (const auto& event : reactor_->events_) {
            event.second->cycle_ = nullptr;
            event.second->id_ = -1;
        }
        reactor_->events_.clear();
        priority_timer().swap(reactor_->ptimer_);

        MSG_TRACE("cycle[{}] stop running...", (void*)this);
    }

    void cycle::exit()
    {
        quit_ = true;

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
            std::lock_guard<std::mutex> guard(functions_mutex_);
            pending_functions_.push_back(std::move(_task));
        }

        if (!in_cycle_thread()) {
            notify();
        }
    }

    auto cycle::queue_size() const -> std::size_t
    {
        std::lock_guard<std::mutex> guard(functions_mutex_);
        return pending_functions_.size();
    }

    void cycle::event_update(const hare::ptr<event>& _event)
    {
        if (_event->owner_cycle() != this && _event->id_ != -1) {
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
                ret = reactor_->event_update(sevent);
            }

            if (!ret) {
                return;
            }

            if (sevent->id_ == -1) {
                sevent->cycle_ = this;
                sevent->id_ = current_thread::get_tds().event_id++;

                assert(reactor_->events_.find(sevent->id_) == reactor_->events_.end());
                reactor_->events_.insert(std::make_pair(sevent->id_, sevent));

                if (sevent->fd() >= 0) {
                    reactor_->inverse_map_.insert(std::make_pair(sevent->fd(), sevent->id_));
                }
            }

            if (CHECK_EVENT(sevent->events_, EVENT_TIMEOUT) != 0) {
                reactor_->ptimer_.emplace(sevent->id_,
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

        if (_event->owner_cycle() != this || _event->id_ == -1) {
            MSG_ERROR("the event is not part of the cycle[{}]", (void*)_event->owner_cycle());
            return;
        }

        run_in_cycle(std::bind([=](const wptr<event>& wevent) {
            auto sevent = wevent.lock();
            if (!sevent) {
                MSG_ERROR("event is empty before it was released.");
                return;
            }
            auto event_id = sevent->id_;
            auto socket = sevent->fd();

            auto iter = reactor_->events_.find(event_id);
            if (iter == reactor_->events_.end()) {
                MSG_ERROR("cannot find event in cycle[{}]", (void*)this);
            } else {
                assert(iter->second == sevent);

                sevent->cycle_ = nullptr;
                sevent->id_ = -1;

                if (socket >= 0) {
                    reactor_->event_remove(sevent);
                    reactor_->inverse_map_.erase(socket);
                }

                reactor_->events_.erase(iter);
            }
        },
            _event));
    }

    auto cycle::event_check(const hare::ptr<event>& _event) -> bool
    {
        if (!_event || _event->id_ < 0) {
            return false;
        }
        assert_in_cycle_thread();
        return reactor_->events_.find(_event->id_) != reactor_->events_.end();
    }

    void cycle::notify()
    {
        if (auto notify = std::static_pointer_cast<detail::event_notify>(notify_event_)) {
            notify->send_notify();
        } else {
            throw exception("cannot get to event_notify.");
        }
    }

    void cycle::abort_not_cycle_thread()
    {
        MSG_FATAL("cycle[{}] was created in thread[{:#x}], current thread is: {:#x}",
            (void*)this, tid_, current_thread::get_tds().tid);
    }

    void cycle::do_pending_functions()
    {
        std::list<task> functions {};
        calling_pending_functions_ = true;

        {
            std::lock_guard<std::mutex> guard(functions_mutex_);
            functions.swap(pending_functions_);
        }

        for (const auto& function : functions) {
            function();
        }

        calling_pending_functions_ = false;
    }

    void cycle::notify_timer()
    {
        auto& priority_event = reactor_->ptimer_;
        auto& events = reactor_->events_;
        const auto revent = EVENT_TIMEOUT;
        auto now = timestamp::now();

        while (!priority_event.empty()) {
            auto top = priority_event.top();
            if (reactor_time_ < top.stamp_) {
                break;
            }
            auto iter = events.find(top.id_);
            if (iter != events.end()) {
                auto& event = iter->second;
                if (event) {
                    MSG_TRACE("event[{}] trigged.", (void*)iter->second.get());
                    event->handle_event(revent, now);
                    if ((event->events_ & EVENT_PERSIST) != 0) {
                        priority_event.emplace(top.id_, timestamp(reactor_time_.microseconds_since_epoch() + event->timeval()));
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

    void register_msg_handler(msg_handler handle)
    {
        msg() = std::move(handle);
    }

} // namespace io
} // namespace hare
