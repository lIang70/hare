#ifndef _HARE_NET_CORE_CYCLE_H_
#define _HARE_NET_CORE_CYCLE_H_

#include <hare/base/thread/thread.h>
#include <hare/base/time/timestamp.h>

#include <list>
#include <vector>

namespace hare {
namespace io {

    class reactor;
    class event;
    class HARE_API cycle : public non_copyable
                         , std::enable_shared_from_this<cycle> {
        timestamp reactor_time_ {};
        thread::id tid_ { 0 };
        bool is_running_ { false };
        bool quit_ { false };
        bool event_handling_ { false };
        bool calling_pending_functions_ { false };

        ptr<event> notify_event_ { nullptr };
        ptr<reactor> reactor_ { nullptr };
        ptr<event> current_active_event_ { nullptr };

        mutable std::mutex functions_mutex_ {};
        std::list<thread::task> pending_functions_ {};

#ifdef HARE_DEBUG
        uint64_t cycle_index_ { 0 };
#endif

    public:
        using REACTOR_TYPE = enum {
            REACTOR_TYPE_EPOLL,
            REACTOR_TYPE_POLL,

            REACTOR_TYPE_NBRS
        };

        explicit cycle(REACTOR_TYPE _type);
        virtual ~cycle();

        /**
         * @brief Time when reactor returns, usually means data arrival.
         *
         **/
        inline auto reactor_return_time() const -> timestamp { return reactor_time_; }
        inline auto event_handling() const -> bool { return event_handling_; }

#ifdef HARE_DEBUG

        inline auto cycle_index() const -> uint64_t
        {
            return cycle_index_;
        }

#endif

        inline void assert_in_cycle_thread()
        {
            if (!in_cycle_thread()) {
                abort_not_cycle_thread();
            }
        }

        auto in_cycle_thread() const -> bool;

        /**
         * @brief loops forever.
         *   Must be called in the same thread as creation of the object.
         **/
        void loop();

        /**
         * @brief Quits cycle.
         *   This is not 100% thread safe, if you call through a raw pointer,
         *   better to call through std::shared_ptr<Cycle> for 100% safety.
         **/
        void exit();

        /**
         * @brief Runs callback immediately in the cycle thread.
         *   It wakes up the cycle, and run the cb.
         *   If in the same cycle thread, cb is run within the function.
         *   Safe to call from other threads.
         **/
        void run_in_cycle(thread::task _task);

        /**
         * @brief Queues callback in the cycle thread.
         *   Runs after finish pooling.
         *   Safe to call from other threads.
         **/
        void queue_in_cycle(thread::task _task);

        auto queue_size() const -> std::size_t;

        void event_update(ptr<event> _event);
        void event_remove(ptr<event> _event);

        /**
         * @brief Detects whether the event is in the reactor.
         *   Must be called in the cycle thread.
         **/
        auto event_check(const ptr<event>& _event) -> bool;

    private:
        void notify();
        void abort_not_cycle_thread();

        void notify_timer();
        void do_pending_functions();
    };

} // namespace io
} // namespace hare

#endif // !_HARE_NET_CORE_CYCLE_H_