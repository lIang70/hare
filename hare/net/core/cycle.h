#ifndef _HARE_NET_CORE_CYCLE_H_
#define _HARE_NET_CORE_CYCLE_H_

#include "hare/base/thread/local.h"
#include <hare/base/util/non_copyable.h>
#include <hare/base/util/thread.h>
#include <hare/base/time/timestamp.h>
#include <hare/net/timer.h>

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <list>

namespace hare {

namespace net {
    class Timer;
}

namespace core {
    class Reactor;
    class Event;

    namespace detail {

        struct TimerInfo {
            net::TimerId timer_id_ {};
            Timestamp timestamp_ {};

            TimerInfo(net::TimerId timer_id, int64_t ms_time)
                : timer_id_(timer_id)
                , timestamp_(ms_time)
            {
            }
        };

        struct TimerPriority {
            bool operator()(const TimerInfo& _x, const TimerInfo& _y)
            {
                return _x.timestamp_ < _y.timestamp_;
            }
        };

        using PriorityTimer = std::priority_queue<TimerInfo, std::vector<TimerInfo>, TimerPriority>;

    } // namespace detail

    class Cycle : public NonCopyable {
    public:
        using EventList = std::vector<Event*>;

    private:
        Timestamp reactor_time_ {};
        Thread::Id tid_ { 0 };
        std::atomic<bool> is_running_ { false };
        std::atomic<bool> quit_ { false };
        std::atomic<bool> event_handling_ { false };
        std::atomic<bool> calling_pending_funcs_ { false };

        std::unique_ptr<Event> notify_event_ { nullptr };
        std::unique_ptr<Reactor> reactor_ { nullptr };

        EventList active_events_ {};
        Event* current_active_event_ { nullptr };

        detail::PriorityTimer priority_timers_ {};
        std::map<net::TimerId, net::Timer*> manage_timers_ {};

        mutable std::mutex mutex_ {};
        std::list<Thread::Task> pending_funcs_ {};

#ifdef HARE_DEBUG
        int64_t cycle_index_ { 0 };
#endif

    public:
        explicit Cycle(const std::string& reactor_type);
        virtual ~Cycle();

        //! @brief Time when reactor returns, usually means data arrival.
        inline Timestamp reactorReturnTime() const { return reactor_time_; }

        inline void assertInCycleThread()
        {
            if (!isInLoopThread()) {
                abortNotInLoopThread();
            }
        }

        inline bool isInLoopThread() const { return tid_ == current_thread::tid(); }

        inline bool eventHandling() const { return event_handling_.load(); }

#ifdef HARE_DEBUG

        inline int64_t cycleIndex() const
        {
            return cycle_index_;
        }

#endif

        //! @brief Loops forever.
        //!
        //!  Must be called in the same thread as creation of the object.
        void loop();

        //! @brief Quits cycle.
        //!
        //!  This is not 100% thread safe, if you call through a raw pointer,
        //!  better to call through std::shared_ptr<Cycle> for 100% safety.
        void exit();

        //! @brief Runs callback immediately in the loop thread.
        //!  It wakes up the loop, and run the cb.
        //!  If in the same loop thread, cb is run within the function.
        //!  Safe to call from other threads.
        void runInLoop(Thread::Task cb);

        //! @brief Queues callback in the loop thread.
        //!  Runs after finish pooling.
        //!  Safe to call from other threads.
        void queueInLoop(Thread::Task cb);

        std::size_t queueSize() const;

        void updateEvent(Event* event);
        void removeEvent(Event* event);
        bool checkEvent(Event* event);

        net::TimerId addTimer(net::Timer* timer);
        void cancel(net::TimerId id);

    private:
        void notify();
        void abortNotInLoopThread();
        void doPendingFuncs();
        void notifyTimer();

        int32_t getWaitTime();

        void printActiveEvents() const;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_CORE_CYCLE_H_