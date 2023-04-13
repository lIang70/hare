#include "hare/base/io/reactor.h"
#include "hare/base/thread/local.h"
#include <hare/base/io/event.h>
#include <hare/base/logging.h>
#include <hare/hare-config.h>

#include <algorithm>
#include <csignal>
#include <mutex>

#ifdef HARE__HAVE_EVENTFD
#include <sys/eventfd.h>
#endif

#ifdef H_OS_UNIX
#include <sys/socket.h>
#endif

namespace hare {
namespace io {
    namespace detail {

        class IgnoreSigPipe {
        public:
            IgnoreSigPipe()
            {
                ::signal(SIGPIPE, SIG_IGN);
            }
        };

        static IgnoreSigPipe s_init_obj {};
        static const int32_t POLL_TIME_MICROSECONDS { 10000 };

        auto create_notify_fd() -> util_socket_t
        {
#ifdef HARE__HAVE_EVENTFD
            auto evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            if (evtfd < 0) {
                SYS_FATAL() << "Failed to get event fd in ::eventfd.";
            }
            return evtfd;
#endif
        }

        class event_notify : public event {
        public:
            explicit event_notify(cycle* cycle)
                : event(cycle, create_notify_fd())
            {
            }

            ~event_notify() override
            {
                ::close(fd());
            }

            void send_notify()
            {
                auto one = static_cast<uint64_t>(1);
                auto write_n = ::send(fd(), &one, sizeof(one), 0);
                if (write_n != sizeof(one)) {
                    SYS_ERROR() << "Write[" << write_n << " B] instead of " << sizeof(one);
                }
            }

            void event_callback(int32_t events, const timestamp& receive_time) override
            {
                H_UNUSED(receive_time);

                if (events == EVENT_READ) {
                    auto one = static_cast<uint64_t>(0);
                    auto read_n = ::recv(fd(), &one, sizeof(one), 0);
                    if (read_n != sizeof(one) && one != static_cast<uint64_t>(1)) {
                        SYS_ERROR() << "Read notify[" << read_n << " B] instead of " << sizeof(one);
                    }
                } else {
                    SYS_FATAL() << "An error occurred while accepting notify in fd[" << fd() << "].";
                }
            }
        };

    } // namespace detail

    cycle::cycle(REACTOR_TYPE _type)
        : tid_(current_thread::tid())
        , notify_event_(new detail::event_notify(this))
        , reactor_(reactor::create_by_type(_type, this))
    {
        LOG_TRACE() << "Cycle[" << this << "] is being initialized in [" << tid_ << "]...";
        if (current_thread::t_data.cycle != nullptr) {
            SYS_FATAL() << "Another Cycle[" << current_thread::t_data.cycle
                        << "] exists in this thread[" << current_thread::tid() << "].";
        } else {
            current_thread::t_data.cycle = this;
        }
        notify_event_->set_flags(EVENT_READ);
    }

    cycle::~cycle()
    {
        pending_functions_.clear();
        notify_event_->clear_all();
        notify_event_.reset();
        current_thread::t_data.cycle = nullptr;
    }

    auto cycle::in_cycle_thread() const -> bool
    {
        return tid_ == current_thread::tid();
    }

    void cycle::loop()
    {
        HARE_ASSERT(!is_running_, "Cycle has been running.");
        is_running_ = true;
        quit_ = false;

        LOG_TRACE() << "Cycle[" << this << "] start running...";

        while (!quit_) {
            active_events_.clear();
            reactor_time_ = reactor_->poll(get_wait_time(), active_events_);

#ifdef HARE_DEBUG
            ++cycle_index_;
#endif

            if (logger::level() <= log::LEVEL_TRACE) {
                print_active_events();
            }

            /// TODO(l1ang): sort event by priority
            event_handling_ = true;

            for (auto& event : active_events_) {
                current_active_event_ = event;
                current_active_event_->handle_event(reactor_time_);
            }
            current_active_event_.reset();

            event_handling_ = false;

            notify_timer();
            do_pending_functions();
        }

        is_running_ = false;

        LOG_TRACE() << "Cycle[" << this << "] stop running.";
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

    void cycle::run_in_cycle(thread::task _task)
    {
        if (in_cycle_thread()) {
            _task();
        } else {
            queue_in_cycle(std::move(_task));
        }
    }

    void cycle::queue_in_cycle(thread::task _task)
    {
        {
            std::lock_guard<std::mutex> guard(functions_mutex_);
            pending_functions_.push_back(std::move(_task));
        }

        if (!in_cycle_thread()) {
            notify();
        }
    }

    auto cycle::queue_size() const -> size_t
    {
        std::lock_guard<std::mutex> guard(functions_mutex_);
        return pending_functions_.size();
    }

    void cycle::event_update(ptr<event> _event)
    {
        HARE_ASSERT(_event->owner_cycle() == this->shared_from_this(), "The event is not part of the cycle.");
        reactor_->event_update(std::move(_event));
    }

    void cycle::event_remove(ptr<event> _event)
    {
        HARE_ASSERT(_event->owner_cycle() == this->shared_from_this(), "The event is not part of the cycle.");
        if (event_handling_) {
            HARE_ASSERT(
                current_active_event_ != _event || std::find(active_events_.begin(), active_events_.end(), _event) == active_events_.end(),
                "Event is actived!");
        }
        reactor_->event_remove(std::move(_event));
    }

    auto cycle::event_check(ptr<event> _event) -> bool
    {
        HARE_ASSERT(_event->owner_cycle() == this->shared_from_this(), "The event is not part of the cycle.");
        return reactor_->event_check(std::move(_event));
    }

    void cycle::notify()
    {
        if (auto* notify = dynamic_cast<detail::event_notify*>(notify_event_.get())) {
            notify->send_notify();
        } else {
            SYS_FATAL() << "Cannot cast to NotifyEvent.";
        }
    }

    void cycle::abort_not_cycle_thread()
    {
        SYS_FATAL() << "Cycle[" << this
                    << "] was created in thread[" << tid_
                    << "], current thread is: " << current_thread::tid();
    }

    void cycle::do_pending_functions()
    {
        std::list<thread::task> functions {};
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
        // while (!priority_timers_.empty()) {
        //     auto top = priority_timers_.top();
        //     if (reactor_time_ < top.timestamp_) {
        //         break;
        //     }
        //     auto iter = manage_timers_.find(top.timer_id_);
        //     if (iter != manage_timers_.end()) {
        //         LOG_TRACE() << "Event[" << top.timer_id_ << "] trigged.";
        //         auto timer = iter->second.lock();
        //         if (timer) {
        //             timer->task()();
        //             if (timer->isPersist()) {
        //                 priority_timers_.emplace(top.timer_id_, reactor_time_.microseconds_since_epoch() + timer->timeout());
        //             } else {
        //                 manage_timers_.erase(iter);
        //             }
        //         } else {
        //             manage_timers_.erase(iter);
        //         }
        //     } else {
        //         LOG_TRACE() << "Event[" << top.timer_id_ << "] deleted.";
        //     }
        //     priority_timers_.pop();
        // }
    }

    auto Cycle::getWaitTime() -> int32_t
    {
        if (priority_timers_.empty()) {
            return detail::POLL_TIME_MICROSECONDS;
        }
        auto time = static_cast<int32_t>(priority_timers_.top().timestamp_.microseconds_since_epoch() - timestamp::now().microseconds_since_epoch());
        return time <= 0 ? static_cast<int32_t>(1) : std::min(time, detail::POLL_TIME_MICROSECONDS);
    }

    void cycle::print_active_events() const
    {
        for (const auto& event : active_events_) {
            LOG_TRACE() << "event[" << event->fd() << "] debug info: " << event->flags_string() << ".";
        }
    }

} // namespace io
} // namespace hare
