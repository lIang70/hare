#include "hare/net/core/cycle.h"
#include "hare/net/core/event.h"
#include "hare/net/core/reactor.h"
#include "hare/net/socket_op.h"
#include <hare/base/logging.h>
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

        socket_t createWakeFd()
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
            NotifyEvent(Cycle* cycle)
                : Event(cycle, createWakeFd())
            {
            }

            ~NotifyEvent() override
            {
                socket::close(fd());
            }

            void sendNotify()
            {
                auto one = (uint64_t)1;
                auto n = socket::write(fd(), &one, sizeof(one));
                if (n != sizeof(one)) {
                    LOG_ERROR() << "Write[" << n << " B] instead of " << sizeof(one);
                }
            }

            void eventCallBack(int32_t events) override
            {
                HARE_ASSERT(ownerCycle() == t_local_cycle, "Cycle is wrong.");

                if (events == EV_READ) {
                    auto one = (uint64_t)0;
                    auto n = socket::read(fd(), &one, sizeof(one));
                    if (n != sizeof(one) && one != (uint64_t)1) {
                        LOG_ERROR() << "Read notify[" << n << " B] instead of " << sizeof(one);
                    }
                } else {
                    SYS_FATAL() << "An error occurred while accepting notify in fd[" << fd() << "].";
                }
            }
        };

    } // namespace detail

    Cycle::Cycle(std::string& reactor_type)
        : notify_event_(new detail::NotifyEvent(this))
        , reactor_(core::Reactor::createByType(reactor_type, this))
        , tid_(current_thread::tid())
    {
        LOG_TRACE() << "Cycle[" << this << "] is being initialized...";
        if (detail::t_local_cycle) {
            LOG_FATAL() << "Another Cycle[" << detail::t_local_cycle
                        << "] exists in this thread[" << tid_ << "].";
        } else {
            detail::t_local_cycle = this;
        }
        notify_event_->setFlags(EV_READ | EV_PERSIST);
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
            reactor_time_ = reactor_->poll(detail::POLL_TIME_MICROSECONDS, active_events_);

#ifdef HARE_DEBUG
            ++cycle_index_;
#endif

            if (Logger::logLevel() <= log::LogLevel::TRACE) {
                printActiveEvents();
            }

            // TODO sort channel by priority
            event_handling_.exchange(true);

            for (auto& event : active_events_) {
                current_active_event_ = event;
                current_active_event_->handleEvent(reactor_time_);
            }
            current_active_event_ = nullptr;

            event_handling_.exchange(false);

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

    void Cycle::runInLoop(Thread::Task cb)
    {
        if (isInLoopThread()) {
            cb();
        } else {
            queueInLoop(std::move(cb));
        }
    }

    void Cycle::queueInLoop(Thread::Task cb)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_funcs_.push_back(std::move(cb));
        }

        if (!isInLoopThread() || calling_pending_funcs_) {
            notify();
        }
    }

    size_t Cycle::queueSize() const
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

    bool Cycle::checkEvent(Event* event)
    {
        HARE_ASSERT(event->ownerCycle() == this, "The event is not part of the cycle.");
        return reactor_->checkEvent(event);
    }

    void Cycle::notify()
    {
        if (auto notify = static_cast<detail::NotifyEvent*>(notify_event_.get()))
            notify->sendNotify();
        else
            SYS_FATAL() << "Cannot cast to NotifyEvent.";
    }

    void Cycle::abortNotInLoopThread()
    {
        LOG_FATAL() << "Cycle[" << this
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

    void Cycle::printActiveEvents() const
    {
        for (const auto& event : active_events_) {
            LOG_TRACE() << "event[" << event->fd() << "] debug info: " << event->flagsToString() << ".";
        }
    }

} // namespace core
} // namespace hare
