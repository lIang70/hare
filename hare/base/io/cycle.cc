#include "hare/base/io/event.h"
#include "hare/base/io/reactor.h"
#include "hare/base/thread/local.h"
#include "hare/base/time/timestamp.h"
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
            explicit event_notify()
                : event(create_notify_fd(), std::bind(&event_notify::event_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), EVENT_READ | EVENT_WRITE | EVENT_PERSIST, 0)
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

            void event_callback(const event::ptr& _event, uint8_t _events, const timestamp& _receive_time)
            {
                H_UNUSED(_receive_time);
                HARE_ASSERT(_event == this->shared_from_this(), "error occurs in event_notify.");

                if (_events == EVENT_READ) {
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

        struct ignore_sigpipe {
            ignore_sigpipe()
            {
                LOG_TRACE() << "Ignore signal[SIGPIPE]";
                ::signal(SIGPIPE, SIG_IGN);
            }
        };

    } // namespace detail

    cycle::cycle(REACTOR_TYPE _type)
        : tid_(current_thread::tid())
        , notify_event_(new detail::event_notify())
        , reactor_(reactor::create_by_type(_type, this))
    {
        // Ignore signal[pipe]
        static detail::ignore_sigpipe s_ignore_sigpipe {};

        LOG_TRACE() << "Cycle[" << this << "] is being initialized in [" << tid_ << "]...";
        if (detail::tstorage.cycle != nullptr) {
            SYS_FATAL() << "Another Cycle[" << detail::tstorage.cycle
                        << "] exists in this thread[" << current_thread::tid() << "].";
        } else {
            detail::tstorage.cycle = this;
        }
    }

    cycle::~cycle()
    {
        // clear thread local data.
        detail::tstorage.active_events.clear();
        detail::tstorage.events.clear();
        detail::priority_timer().swap(detail::tstorage.ptimer);
        pending_functions_.clear();
        notify_event_.reset();
        detail::tstorage.cycle = nullptr;
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
        event_add(notify_event_);

        LOG_TRACE() << "Cycle[" << this << "] start running...";

        while (!quit_) {
            detail::tstorage.active_events.clear();

            reactor_time_ = reactor_->poll(detail::get_wait_time());

#ifdef HARE_DEBUG
            ++cycle_index_;
#endif

            if (logger::level() <= log::LEVEL_TRACE) {
                detail::print_active_events();
            }

            /// TODO(l1ang): sort event by priority
            event_handling_ = true;

            for (auto& event_elem : detail::tstorage.active_events) {
                current_active_event_ = event_elem.event;
                current_active_event_->handle_event(event_elem.revents, reactor_time_);
                if (CHECK_EVENT(current_active_event_->event_flag_, EVENT_PERSIST) == 0) {
                    event_remove(current_active_event_);
                }
            }
            current_active_event_.reset();

            event_handling_ = false;

            notify_timer();
            do_pending_functions();
        }

        notify_event_->del();
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

    void cycle::event_add(ptr<event> _event)
    {
        if (!_event->owner_cycle() && _event->id_ != -1) {
            SYS_ERROR() << "Cannot add event from other cycle[" << _event->owner_cycle().get() << "]";
            return;
        }

        run_in_cycle(std::bind([=] (const wptr<event>& wevent) {
            auto sevent = wevent.lock();
            if (sevent) {
                return ;
            }

            sevent->cycle_ = this->shared_from_this();
            sevent->id_ = detail::tstorage.event_id++;

            HARE_ASSERT(detail::tstorage.events.find(sevent->id_) == detail::tstorage.events.end(), "");

            detail::tstorage.events.insert(std::make_pair(sevent->id_, sevent));
            
            if (CHECK_EVENT(sevent->event_flag_, EVENT_TIMEOUT)) {
                detail::tstorage.ptimer.emplace(sevent->id_, timestamp::now().microseconds_since_epoch() + sevent->timeval());
            }

            if (sevent->fd() >= 0) {
                reactor_->event_add(sevent);
            }

        }, _event));
    }

    void cycle::event_remove(ptr<event> _event)
    {
        if (!_event) {
            return ;
        }

        if (_event->owner_cycle() != this->shared_from_this() || _event->id_ == -1) {
            SYS_ERROR() << "The event is not part of the cycle[" << _event->owner_cycle().get() << "]";
            return;
        }

        run_in_cycle(std::bind([=] (const wptr<event>& wevent) {
            auto sevent = wevent.lock();
            if (sevent) {
                SYS_ERROR() << "Event is empty before it was released.";
                return ;
            }
            auto event_id = sevent->id_;
            auto socket = sevent->fd();

            auto iter = detail::tstorage.events.find(event_id);
            if (iter == detail::tstorage.events.end()) {
                SYS_ERROR() << "Cannot find event in cycle[" << this << "]";
            } else {
                HARE_ASSERT(iter->second == sevent, "");

                detail::tstorage.events.erase(iter);

                if (socket >= 0) {
                    reactor_->event_remove(sevent);
                }

                sevent->cycle_.reset();
                sevent->id_ = -1;
            }

        }, _event));
    }

    auto cycle::event_check(const ptr<event>& _event) -> bool
    {
        if (!_event || _event->id_ < 0) {
            return false;
        }
        assert_in_cycle_thread();
        return detail::tstorage.events.find(_event->id_) != detail::tstorage.events.end();
    }

    void cycle::notify()
    {
        if (auto* notify = implicit_cast<detail::event_notify*>(notify_event_.get())) {
            notify->send_notify();
        } else {
            SYS_FATAL() << "Cannot get to event_notify.";
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
        auto& priority_event = detail::tstorage.ptimer;
        auto& events = detail::tstorage.events;
        const auto revent = EVENT_TIMEOUT;
        auto now = timestamp::now();

        while (!priority_event.empty()) {
            auto top = priority_event.top();
            if (reactor_time_ < top.stamp) {
                break;
            }
            auto iter = events.find(top.id);
            if (iter != events.end()) {
                auto& event = iter->second;
                if (event) {
                    LOG_TRACE() << "Event[" << iter->second.get() << "] trigged.";
                    event->handle_event(revent, now);
                    if ((event->event_flag_ & EVENT_PERSIST) != 0) {
                        priority_event.emplace(top.id, reactor_time_.microseconds_since_epoch() + event->timeval());
                    } else {
                        event_remove(event);
                    }
                } else {
                    events.erase(iter);
                }
            } else {
                LOG_TRACE() << "Event[" << top.id << "] deleted.";
            }
            priority_event.pop();
        }
    }

} // namespace io
} // namespace hare
