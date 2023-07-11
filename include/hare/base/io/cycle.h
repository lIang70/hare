/**
 * @file hare/base/io/cycle.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with cycle.h
 * @version 0.1-beta
 * @date 2023-04-14
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_IO_CYCLE_H_
#define _HARE_BASE_IO_CYCLE_H_

#include <hare/base/util/non_copyable.h>
#include <hare/base/time/timestamp.h>

#include <list>
#include <mutex>

namespace hare {
namespace io {

    class reactor;
    class event;
    HARE_CLASS_API
    class HARE_API cycle : public util::non_copyable
                         , public std::enable_shared_from_this<cycle> {
        timestamp reactor_time_ {};
        std::uint64_t tid_ { 0 };
        bool is_running_ { false };
        bool quit_ { false };
        bool event_handling_ { false };
        bool calling_pending_functions_ { false };

        ptr<event> notify_event_ { nullptr };
        ptr<reactor> reactor_ { nullptr };
        ptr<event> current_active_event_ { nullptr };

        mutable std::mutex functions_mutex_ {};
        std::list<task> pending_functions_ {};

#if HARE_DEBUG
        uint64_t cycle_index_ { 0 };
#endif

    public:
        using ptr = ptr<cycle>;

        using REACTOR_TYPE = enum {
            REACTOR_TYPE_EPOLL,
            REACTOR_TYPE_POLL,

            REACTOR_TYPE_NBRS
        };

        explicit cycle(REACTOR_TYPE _type);
        virtual ~cycle();

        /**
         * @brief Time when reactor returns, usually means data arrival.
         **/
        HARE_INLINE
        auto reactor_return_time() const -> timestamp { return reactor_time_; }
        HARE_INLINE
        auto event_handling() const -> bool { return event_handling_; }
        HARE_INLINE
        auto is_running() const -> bool { return is_running_; }

#if HARE_DEBUG

        HARE_INLINE
        auto cycle_index() const -> uint64_t
        {
            return cycle_index_;
        }

#endif

        HARE_INLINE
        void assert_in_cycle_thread()
        {
            if (!in_cycle_thread()) {
                abort_not_cycle_thread();
            }
        }

        auto in_cycle_thread() const -> bool;

        auto type() const -> REACTOR_TYPE;

        /**
         * @brief Loops forever.
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
        void run_in_cycle(task _task);

        /**
         * @brief Queues callback in the cycle thread.
         *   Runs after finish pooling.
         *   Safe to call from other threads.
         **/
        void queue_in_cycle(task _task);

        auto queue_size() const -> std::size_t;

        void event_update(const hare::ptr<event>& _event);
        void event_remove(const hare::ptr<event>& _event);

        /**
         * @brief Detects whether the event is in the reactor.
         *   Must be called in the cycle thread.
         **/
        auto event_check(const hare::ptr<event>& _event) -> bool;

    private:
        void notify();
        void abort_not_cycle_thread();

        void notify_timer();
        void do_pending_functions();
    };

    using msg_handler = std::function<void(std::string)>;

    HARE_API void register_msg_handler(msg_handler handle);

} // namespace io
} // namespace hare

#endif // !_HARE_BASE_IO_CYCLE_H_