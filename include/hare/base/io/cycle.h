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

#include <hare/base/time/timestamp.h>
#include <hare/base/util/non_copyable.h>

namespace hare {
namespace io {

    class Reactor;
    class Event;
    HARE_CLASS_API
    class HARE_API Cycle : public util::NonCopyable {
        hare::detail::Impl* impl_ {};

    public:
        using REACTOR_TYPE = enum {
            REACTOR_TYPE_EPOLL,
            REACTOR_TYPE_POLL,

            REACTOR_TYPE_NBRS
        };

        explicit Cycle(REACTOR_TYPE _type);
        virtual ~Cycle();

        /**
         * @brief Time when reactor returns, usually means data arrival.
         **/
        auto ReactorReturnTime() const -> Timestamp;
        auto event_handling() const -> bool;
        auto is_running() const -> bool;
        auto type() const -> REACTOR_TYPE;

#if HARE_DEBUG

        auto cycle_index() const -> std::uint64_t;

#endif

        HARE_INLINE
        void AssertInCycleThread()
        {
            if (!InCycleThread()) {
                AbortNotCycleThread();
            }
        }

        auto InCycleThread() const -> bool;

        /**
         * @brief Loops forever.
         *   Must be called in the same thread as creation of the object.
         **/
        void Exec();

        /**
         * @brief Quits cycle.
         *   This is not 100% thread safe, if you call through a raw pointer,
         *   better to call through std::shared_ptr<Cycle> for 100% safety.
         **/
        void Exit();

        /**
         * @brief Runs callback immediately in the cycle thread.
         *   It wakes up the cycle, and run the cb.
         *   If in the same cycle thread, cb is run within the function.
         *   Safe to call from other threads.
         **/
        void RunInCycle(Task _task);

        /**
         * @brief Queues callback in the cycle thread.
         *   Runs after finish pooling.
         *   Safe to call from other threads.
         **/
        void QueueInCycle(Task _task);

        auto queue_size() const -> std::size_t;

        void EventUpdate(const hare::Ptr<Event>& _event);
        void EventRemove(const hare::Ptr<Event>& _event);

        /**
         * @brief Detects whether the event is in the reactor.
         *   Must be called in the cycle thread.
         **/
        auto EventCheck(const hare::Ptr<Event>& _event) -> bool;

    private:
        void Notify();
        void AbortNotCycleThread();

        void NotifyTimer();
        void DoPendingFunctions();
    };

} // namespace io
} // namespace hare

#endif // _HARE_BASE_IO_CYCLE_H_
