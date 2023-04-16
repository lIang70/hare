#include "hare/base/io/reactor.h"

#include "hare/base/thread/local.h"
#include "hare/base/time/timestamp.h"
#include <hare/base/logging.h>
#include <hare/hare-config.h>

#include <algorithm>
#include <csignal>
#include <memory>
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
                SYS_FATAL() << "failed to get event fd in ::eventfd.";
            }
            return evtfd;
#endif
        }

        class event_notify : public event {
        public:
            explicit event_notify()
                : event(create_notify_fd(),
                    std::bind(&event_notify::event_callback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
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
                uint64_t one = 1;
                auto write_n = ::send(fd(), &one, sizeof(one), 0);
                if (write_n != sizeof(one)) {
                    SYS_ERROR() << "write[" << write_n << " B] instead of " << sizeof(one);
                }
            }

            void event_callback(const event::ptr& _event, uint8_t _events, const timestamp& _receive_time)
            {
                H_UNUSED(_receive_time);
                HARE_ASSERT(_event == this->shared_from_this(), "error occurs in event_notify.");

                if (CHECK_EVENT(_events, EVENT_READ) != 0) {
                    uint64_t one = 0;
                    auto read_n = ::recv(fd(), &one, sizeof(one), 0);
                    if (read_n != sizeof(one) && one != static_cast<uint64_t>(1)) {
                        SYS_ERROR() << "read notify[" << read_n << " B] instead of " << sizeof(one);
                    }
                } else {
                    SYS_FATAL() << "an error occurred while accepting notify in fd[" << fd() << "].";
                }
            }
        };

        struct ignore_sigpipe {
            ignore_sigpipe()
            {
                LOG_TRACE() << "ignore signal[SIGPIPE]";
                ::signal(SIGPIPE, SIG_IGN);
            }
        };

    } // namespace detail

    auto get_wait_time() -> int32_t
    {
        const auto& tstorage = current_thread::tstorage;
        if (tstorage.ptimer.empty()) {
            return POLL_TIME_MICROSECONDS;
        }
        auto time = static_cast<int32_t>(tstorage.ptimer.top().stamp_.microseconds_since_epoch() - timestamp::now().microseconds_since_epoch());
        return time <= 0 ? static_cast<int32_t>(1) : std::min(time, POLL_TIME_MICROSECONDS);
    }

    void print_active_events()
    {
        const auto& tstorage = current_thread::tstorage;
        for (const auto& event_elem : tstorage.active_events) {
            LOG_TRACE() << "event[" << event_elem.event_->fd() << "] debug info: " << event_elem.event_->event_string() << ".";
        }
    }

    cycle::cycle(REACTOR_TYPE _type)
        : tid_(current_thread::tid())
        , notify_event_(new detail::event_notify())
        , reactor_(reactor::create_by_type(_type, this))
    {
        // Ignore signal[pipe]
        static detail::ignore_sigpipe s_ignore_sigpipe {};

        LOG_TRACE() << "cycle[" << this << "] is being initialized in [" << tid_ << "]...";
        if (current_thread::tstorage.cycle != nullptr) {
            SYS_FATAL() << "another cycle[" << current_thread::tstorage.cycle
                        << "] exists in this thread[" << current_thread::tid() << "].";
        } else {
            current_thread::tstorage.cycle = this;
        }
    }

    cycle::~cycle()
    {
        // clear thread local data.
        current_thread::tstorage.active_events.clear();
        current_thread::tstorage.events.clear();
        priority_timer().swap(current_thread::tstorage.ptimer);
        pending_functions_.clear();
        notify_event_.reset();
        current_thread::tstorage.cycle = nullptr;
    }

    auto cycle::in_cycle_thread() const -> bool
    {
        return tid_ == current_thread::tid();
    }

    void cycle::loop()
    {
        HARE_ASSERT(!is_running_, "cycle has been running.");
        is_running_ = true;
        quit_ = false;
        event_update(notify_event_);

        LOG_TRACE() << "cycle[" << this << "] start running...";

        while (!quit_) {
            current_thread::tstorage.active_events.clear();

            reactor_time_ = reactor_->poll(get_wait_time());

#ifdef HARE_DEBUG
            ++cycle_index_;
#endif

            if (logger::level() <= log::LEVEL_TRACE) {
                print_active_events();
            }

            /// TODO(l1ang): sort event by priority
            event_handling_ = true;

            for (auto& event_elem : current_thread::tstorage.active_events) {
                current_active_event_ = event_elem.event_;
                current_active_event_->handle_event(event_elem.revents_, reactor_time_);
                if (CHECK_EVENT(current_active_event_->events_, EVENT_PERSIST) == 0) {
                    event_remove(current_active_event_);
                }
            }
            current_active_event_.reset();

            event_handling_ = false;

            notify_timer();
            do_pending_functions();
        }

        notify_event_->deactivate();
        is_running_ = false;

        LOG_TRACE() << "cycle[" << this << "] stop running.";
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
        if (in_cycle_thread() || !is_running()) {
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

    auto cycle::queue_size() const -> size_t
    {
        std::lock_guard<std::mutex> guard(functions_mutex_);
        return pending_functions_.size();
    }

    void cycle::event_update(ptr<event> _event)
    {
        if (_event->owner_cycle() != shared_from_this() && _event->id_ != -1) {
            SYS_ERROR() << "cannot add event from other cycle[" << _event->owner_cycle().get() << "]";
            return;
        }

        run_in_cycle(std::bind([=](const wptr<event>& wevent) {
            auto sevent = wevent.lock();
            if (!sevent) {
                return;
            }

            if (sevent->fd() >= 0) {
                reactor_->event_update(sevent);
            }

            if (sevent->id_ == -1) {
                sevent->cycle_ = this->shared_from_this();
                sevent->id_ = current_thread::tstorage.event_id++;

                HARE_ASSERT(current_thread::tstorage.events.find(sevent->id_) == current_thread::tstorage.events.end(), "an error occurred while creating the event id.");
                current_thread::tstorage.events.insert(std::make_pair(sevent->id_, sevent));

                if (sevent->fd() >= 0) {
                    current_thread::tstorage.inverse_map.insert(std::make_pair(sevent->fd(), sevent->id_));
                }
            }

            if (CHECK_EVENT(sevent->events_, EVENT_TIMEOUT) != 0) {
                current_thread::tstorage.ptimer.emplace(sevent->id_, timestamp(timestamp::now().microseconds_since_epoch() + sevent->timeval()));
            }
        },
            _event));
    }

    void cycle::event_remove(ptr<event> _event)
    {
        if (!_event) {
            return;
        }

        if (_event->owner_cycle() != this->shared_from_this() || _event->id_ == -1) {
            SYS_ERROR() << "the event is not part of the cycle[" << _event->owner_cycle().get() << "]";
            return;
        }

        run_in_cycle(std::bind([=](const wptr<event>& wevent) {
            auto sevent = wevent.lock();
            if (!sevent) {
                SYS_ERROR() << "event is empty before it was released.";
                return;
            }
            auto event_id = sevent->id_;
            auto socket = sevent->fd();

            auto iter = current_thread::tstorage.events.find(event_id);
            if (iter == current_thread::tstorage.events.end()) {
                SYS_ERROR() << "cannot find event in cycle[" << this << "]";
            } else {
                HARE_ASSERT(iter->second == sevent, "error event.");

                sevent->cycle_.reset();
                sevent->id_ = -1;

                if (socket >= 0) {
                    reactor_->event_remove(sevent);
                    current_thread::tstorage.inverse_map.erase(socket);
                }

                current_thread::tstorage.events.erase(iter);
            }
        },
            _event));
    }

    auto cycle::event_check(const ptr<event>& _event) -> bool
    {
        if (!_event || _event->id_ < 0) {
            return false;
        }
        assert_in_cycle_thread();
        return current_thread::tstorage.events.find(_event->id_) != current_thread::tstorage.events.end();
    }

    void cycle::notify()
    {
        if (auto notify = std::static_pointer_cast<detail::event_notify>(notify_event_)) {
            notify->send_notify();
        } else {
            SYS_FATAL() << "cannot get to event_notify.";
        }
    }

    void cycle::abort_not_cycle_thread()
    {
        SYS_FATAL() << "cycle[" << this
                    << "] was created in thread[" << tid_
                    << "], current thread is: " << current_thread::tid();
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
        auto& priority_event = current_thread::tstorage.ptimer;
        auto& events = current_thread::tstorage.events;
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
                    LOG_TRACE() << "event[" << iter->second.get() << "] trigged.";
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
                LOG_TRACE() << "event[" << top.id_ << "] deleted.";
            }
            priority_event.pop();
        }
    }

} // namespace io
} // namespace hare
