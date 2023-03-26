#include "hare/net/core/cycle.h"
#include "hare/base/util/util.h"
#include "hare/net/core/event.h"
#include "hare/net/core/reactor.h"
#include "hare/net/socket_op.h"
#include <hare/base/logging.h>
#include <hare/net/timer.h>
#include <hare/net/util.h>

#include <algorithm>

#ifdef HARE__HAVE_EVENTFD
#include <sys/eventfd.h>
#endif

namespace hare {
namespace core {
    namespace detail {

        const int32_t POLL_TIME_MICROSECONDS { 10000 };
        thread_local Cycle* t_local_cycle { nullptr };
        std::atomic<net::TimerId> s_timer_id { 1 };

        auto createWakeFd() -> util_socket_t
        {
#ifdef HARE__HAVE_EVENTFD
            auto evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            if (evtfd < 0) {
                SYS_FATAL() << "Failed in ::eventfd";
            }
            return evtfd;
#endif
        }

        class NotifyEvent : public Event {
        public:
            explicit NotifyEvent(Cycle* cycle)
                : Event(cycle, createWakeFd())
            {
            }

            ~NotifyEvent() override
            {
                socket::close(fd());
            }

            void sendNotify()
            {
                auto one = static_cast<uint64_t>(1);
                auto write_n = socket::write(fd(), &one, sizeof(one));
                if (write_n != sizeof(one)) {
                    SYS_ERROR() << "Write[" << write_n << " B] instead of " << sizeof(one);
                }
            }

            void eventCallBack(int32_t events, const Timestamp& receive_time) override
            {
                HARE_ASSERT(ownerCycle() == t_local_cycle, "Cycle is wrong.");

                H_UNUSED(receive_time);

                if (events == net::EVENT_READ) {
                    auto one = static_cast<uint64_t>(0);
                    auto read_n = socket::read(fd(), &one, sizeof(one));
                    if (read_n != sizeof(one) && one != static_cast<uint64_t>(1)) {
                        SYS_ERROR() << "Read notify[" << read_n << " B] instead of " << sizeof(one);
                    }
                } else {
                    SYS_FATAL() << "An error occurred while accepting notify in fd[" << fd() << "].";
                }
            }
        };

    } // namespace detail

    Cycle::Cycle(const std::string& reactor_type)
        : notify_event_(new detail::NotifyEvent(this))
        , reactor_(Reactor::createByType(reactor_type, this))
        , tid_(current_thread::tid())
    {
        LOG_TRACE() << "Cycle[" << this << "] is being initialized...";
        if (detail::t_local_cycle != nullptr) {
            SYS_FATAL() << "Another Cycle[" << detail::t_local_cycle
                        << "] exists in this thread[" << tid_ << "].";
        } else {
            detail::t_local_cycle = this;
        }
        notify_event_->setFlags(net::EVENT_READ);
    }

    Cycle::~Cycle()
    {
        assertInCycleThread();
        notify_event_->clearAllFlags();
        notify_event_.reset();
        detail::t_local_cycle = nullptr;
    }

    void Cycle::loop()
    {
        HARE_ASSERT(!is_running_, "Cycle has been running.");
        assertInCycleThread();
        is_running_.exchange(true);
        quit_.exchange(false);

        LOG_TRACE() << "Cycle[" << this << "] start running...";

        while (!quit_) {
            active_events_.clear();
            reactor_time_ = reactor_->poll(getWaitTime(), active_events_);

#ifdef HARE_DEBUG
            ++cycle_index_;
#endif

            if (Logger::logLevel() <= log::LogLevel::TRACE) {
                printActiveEvents();
            }

            // TODO sort event by priority
            event_handling_.exchange(true);

            for (auto& event : active_events_) {
                current_active_event_ = event;
                current_active_event_->handleEvent(reactor_time_);
            }
            current_active_event_ = nullptr;

            event_handling_.exchange(false);

            notifyTimer();
            doPendingFuncs();
        }

        is_running_.exchange(false);

        LOG_TRACE() << "Cycle[" << this << "] stop running.";
    }

    void Cycle::exit()
    {
        quit_ = true;

        //! @brief There is a chance that loop() just executes while(!quit_) and exits,
        //!  then Cycle destructs, then we are accessing an invalid object.
        if (!isInLoopThread()) {
            notify();
        }
    }

    void Cycle::runInLoop(Thread::Task task)
    {
        if (isInLoopThread()) {
            task();
        } else {
            queueInLoop(std::move(task));
        }
    }

    void Cycle::queueInLoop(Thread::Task task)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_funcs_.push_back(std::move(task));
        }

        if (!isInLoopThread() || calling_pending_funcs_) {
            notify();
        }
    }

    auto Cycle::queueSize() const -> size_t
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_funcs_.size();
    }

    void Cycle::updateEvent(Event* event)
    {
        HARE_ASSERT(event->ownerCycle() == this, "The event is not part of the cycle.");
        assertInCycleThread();
        reactor_->updateEvent(event);
    }

    void Cycle::removeEvent(Event* event)
    {
        HARE_ASSERT(event->ownerCycle() == this, "The event is not part of the cycle.");
        assertInCycleThread();
        if (event_handling_) {
            HARE_ASSERT(
                current_active_event_ != event || std::find(active_events_.begin(), active_events_.end(), event) == active_events_.end(),
                "Event is actived!");
        }
        reactor_->removeEvent(event);
    }

    auto Cycle::checkEvent(Event* event) -> bool
    {
        HARE_ASSERT(event->ownerCycle() == this, "The event is not part of the cycle.");
        return reactor_->checkEvent(event);
    }

    auto Cycle::addTimer(net::Timer* timer) -> net::TimerId
    {
        assertInCycleThread();
        auto timer_id = detail::s_timer_id.fetch_add(1);
        manage_timers_.insert(std::make_pair(timer_id, timer));
        priority_timers_.emplace(timer_id, Timestamp::now().microSecondsSinceEpoch() + timer->timeout());
        return timer_id;
    }

    void Cycle::cancel(net::TimerId timer_id)
    {
        assertInCycleThread();
        manage_timers_.erase(timer_id);
    }

    void Cycle::notify()
    {
        if (auto* notify = dynamic_cast<detail::NotifyEvent*>(notify_event_.get())) {
            notify->sendNotify();
        } else {
            SYS_FATAL() << "Cannot cast to NotifyEvent.";
        }
    }

    void Cycle::abortNotInLoopThread()
    {
        SYS_FATAL() << "Cycle[" << this
                    << "] was created in thread[" << tid_
                    << "], current thread is: " << current_thread::tid();
    }

    void Cycle::doPendingFuncs()
    {
        std::list<Thread::Task> funcs;
        calling_pending_funcs_.exchange(true);

        {
            std::lock_guard<std::mutex> lock(mutex_);
            funcs.swap(pending_funcs_);
        }

        for (const auto& functor : funcs) {
            functor();
        }

        calling_pending_funcs_.exchange(false);
    }

    void Cycle::notifyTimer()
    {
        while (!priority_timers_.empty()) {
            auto top = priority_timers_.top();
            if (reactor_time_ < top.timestamp_) {
                break;
            }
            auto iter = manage_timers_.find(top.timer_id_);
            if (iter != manage_timers_.end()) {
                iter->second->task()();
                if (iter->second->isPersist()) {
                    priority_timers_.emplace(top.timer_id_, reactor_time_.microSecondsSinceEpoch() + iter->second->timeout());
                } else {
                    manage_timers_.erase(iter);
                }
            }
            priority_timers_.pop();
        }
    }

    auto Cycle::getWaitTime() -> int32_t
    {
        if (priority_timers_.empty()) {
            return detail::POLL_TIME_MICROSECONDS;
        }
        auto time = static_cast<int32_t>(priority_timers_.top().timestamp_.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch());
        return time <= 0 ? static_cast<int32_t>(1) : std::min(time, detail::POLL_TIME_MICROSECONDS);
    }

    void Cycle::printActiveEvents() const
    {
        for (const auto& event : active_events_) {
            LOG_TRACE() << "event[" << event->fd() << "] debug info: " << event->flagsToString() << ".";
        }
    }

} // namespace core
} // namespace hare
