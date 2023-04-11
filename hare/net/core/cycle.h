#ifndef _HARE_NET_CORE_CYCLE_H_
#define _HARE_NET_CORE_CYCLE_H_

#include <hare/base/time/timestamp.h>
#include <hare/base/util/non_copyable.h>
#include <hare/net/core/timer.h>

#include <list>
#include <map>
#include <queue>

namespace hare {
namespace core {

    class Reactor;
    class Event;

    namespace detail {

        struct TimerInfo {
            Timer::Id timer_id_ {};
            Timestamp timestamp_ {};

            TimerInfo(Timer::Id timer_id, int64_t ms_time)
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
        bool is_running_ { false };
        bool quit_ { false };
        bool event_handling_ { false };
        bool calling_pending_functions_ { false };

        std::unique_ptr<Event> notify_event_ { nullptr };
        std::unique_ptr<Reactor> reactor_ { nullptr };

        EventList active_events_ {};
        Event* current_active_event_ { nullptr };

        detail::PriorityTimer priority_timers_ {};
        std::map<Timer::Id, Timer*> manage_timers_ {};

        std::list<Timer::Task> pending_functions_ {};

#ifdef HARE_DEBUG
        int64_t cycle_index_ { 0 };
#endif

    public:
        explicit Cycle(const std::string& reactor_type);
        virtual ~Cycle();

        /**
         * @brief Time when reactor returns, usually means data arrival.
         *
         */
        inline auto reactorReturnTime() const -> Timestamp { return reactor_time_; }

        inline auto eventHandling() const -> bool { return event_handling_; }

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
         *   better to call through std::shared_ptr<Cycle> for 100% safety.
         */
        void exit();

        /**
         * @brief Runs callback immediately in the loop thread.
         *   It wakes up the loop, and run the cb.
         *   If in the same loop thread, cb is run within the function.
         *   Safe to call from other threads.
         */
        void runInLoop(Timer::Task task);

        /**
         * @brief Queues callback in the loop thread.
         *   Runs after finish pooling.
         *   Safe to call from other threads.
         */
        void queueInLoop(Timer::Task task);

        auto queueSize() const -> std::size_t;

        void updateEvent(Event* event);
        void removeEvent(Event* event);
        auto checkEvent(Event* event) -> bool;

        auto addTimer(Timer* timer) -> Timer::Id;
        void cancel(Timer::Id timer_id);

    private:
        void notify();
        void doPendingFunctions();
        void notifyTimer();

        auto getWaitTime() -> int32_t;

        void printActiveEvents() const;
    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_CORE_CYCLE_H_