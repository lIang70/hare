#include "hare/net/core/cycle.h"
#include "hare/net/core/event.h"
#include "hare/net/core/reactor.h"
#include "hare/net/socket_op.h"
#include <hare/base/logging.h>
#include <hare/net/util.h>

#ifdef H_OS_WIN32
#else
#include <sys/socket.h>
#endif

namespace hare {
namespace core {
    namespace detail {

        const int32_t pool_time_micros { 10000 };
        thread_local Cycle* t_local_cycle { nullptr };

    } // namespace detail

    Cycle::Cycle(std::string& reactor_type)
        : wake_up_event_(new Event)
        , reactor_(core::Reactor::createByType(reactor_type, this))
    {
        LOG_TRACE() << "Cycle[" << this << "] is being initialized...";
        wake_up_event_->cycle_ = this;
        wake_up_event_->fd_ = socket::createNonblockingOrDie(AF_INET);
        wake_up_event_->setFlags(EV_READ | EV_PERSIST);
    }

    Cycle::~Cycle()
    {
        wake_up_event_->clearAllFlags();
        wake_up_event_->deactive();
        wake_up_event_.reset();
    }

    void Cycle::loop()
    {
        HARE_ASSERT(!is_running_, "Cycle has been running.");
        is_running_.exchange(true);
        quit_.exchange(false);
        detail::t_local_cycle = this;

        LOG_TRACE() << "Cycle[" << this << "] start running...";

        while (!quit_) {
            active_events_.clear();
            reactor_time_ = reactor_->poll(detail::pool_time_micros, active_events_);

#ifdef HARE_DEBUG
            ++cycle_index_;
#endif

            if (Logger::logLevel() <= log::LogLevel::TRACE) {
                printActiveEvents();
            }

            // TODO sort channel by priority
            event_handling_.exchange(true);

            for (auto& event : active_events_) {
                current_active_event_ = event.lock();
                current_active_event_->handleEvent(reactor_time_);
            }
            current_active_event_ = nullptr;

            event_handling_.exchange(false);

            doPendingFuncs();
        }

        is_running_.exchange(false);
        detail::t_local_cycle = nullptr;

        LOG_TRACE() << "Cycle[" << this << "] stop running.";
    }

    void Cycle::exit()
    {
        quit_ = true;

        //! @brief There is a chance that loop() just executes while(!quit_) and exits,
        //!  then Cycle destructs, then we are accessing an invalid object.
        if (!isInLoopThread()) {
            wakeup();
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
            wakeup();
        }
    }

    size_t Cycle::queueSize() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pending_funcs_.size();
    }

    void Cycle::wakeup()
    {
        uint64_t one = 1;
        ssize_t n = socket::write(wake_up_event_->fd_, &one, sizeof(one));
        if (n != sizeof(one)) {
            LOG_ERROR() << "Cycle::wakeup() writes " << n << " bytes instead of " << sizeof(one);
        }
    }

    void Cycle::updateEvent(std::shared_ptr<Event>& event)
    {
        HARE_ASSERT(event->ownerCycle() == this, "The event is not part of the cycle.");
        assertInCycleThread();
        reactor_->updateEvent(event);
    }

    void Cycle::removeEvent(std::shared_ptr<Event>& event)
    {
        HARE_ASSERT(event->ownerCycle() == this, "The event is not part of the cycle.");
        assertInCycleThread();
        if (event_handling_) {
            HARE_ASSERT(current_active_event_ != event, "Event is actived!");
        }
        reactor_->removeEvent(event);
    }

    bool Cycle::checkEvent(std::shared_ptr<Event>& event)
    {
        HARE_ASSERT(event->ownerCycle() == this, "The event is not part of the cycle.");
        assertInCycleThread();
        return reactor_->checkEvent(event);
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
            if (auto e = event.lock())
                LOG_TRACE() << "event[" << e->fd() << "] debug info: " << e->flagsToString() << ".";
        }
    }

} // namespace core
} // namespace hare
