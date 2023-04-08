#ifndef _HARE_NET_CORE_CYCLE_H_
#define _HARE_NET_CORE_CYCLE_H_

#include "hare/base/thread/local.h"
#include <hare/base/time/timestamp.h>
#include <hare/base/util/non_copyable.h>
#include <hare/base/util/thread.h>
#include <hare/net/timer.h>

#include <atomic>
#include <list>
#include <map>
#include <mutex>
#include <queue>

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
            auto operator()(const TimerInfo& time_x, const TimerInfo& time_y) -> bool
            {
                return time_x.timestamp_ < time_y.timestamp_;
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
        using Ptr = std::shared_ptr<Cycle>;

        explicit Cycle(const std::string& reactor_type);
        virtual ~Cycle();

        /**
         * @brief Time when reactor returns, usually means data arrival.
         * 
         */
        inline auto reactorReturnTime() const -> Timestamp { return reactor_time_; }
        inline void assertInCycleThread()
        {
            if (!isInCycleThread()) {
                abortNotInLoopThread();
            }
        }
        inline auto isInCycleThread() const -> bool { return tid_ == current_thread::tid(); }
        inline auto eventHandling() const -> bool { return event_handling_.load(); }

#ifdef HARE_DEBUG

        inline auto cycleIndex() const -> int64_t
        {
            return cycle_index_;
        }

#endif

        /**
         * @brief Loops forever.
         *   Must be called in the same thread as creation of the object.
         */
        void loop();

        /**
         * @brief Quits cycle.
         *   This is not 100% thread safe, if you call through a raw pointer,
         *  better to call through std::shared_ptr<Cycle> for 100% safety.
         */
        void exit();

        /**
         * @brief Runs callback immediately in the loop thread.
         *   It wakes up the loop, and run the cb.
         *   If in the same loop thread, cb is run within the function.
         *   Safe to call from other threads. 
         */
        void runInLoop(Thread::Task task);

        /**
         * @brief Queues callback in the loop thread.
         *   Runs after finish pooling.
         *   Safe to call from other threads. 
         */
        void queueInLoop(Thread::Task task);

        auto queueSize() const -> std::size_t;

        void updateEvent(Event* event);
        void removeEvent(Event* event);
        auto checkEvent(Event* event) -> bool;

        auto addTimer(net::Timer* timer) -> net::TimerId;
        void cancel(net::TimerId timer_id);

    private:
        void notify();
        void abortNotInLoopThread();
        void doPendingFunctions();
        void notifyTimer();

        auto getWaitTime() -> int32_t;

        void printActiveEvents() const;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_CORE_CYCLE_H_