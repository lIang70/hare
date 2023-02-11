#ifndef _HARE_NET_CYCLE_H_
#define _HARE_NET_CYCLE_H_

#include "hare/base/thread/local.h"
#include <hare/base/detail/non_copyable.h>
#include <hare/base/thread.h>
#include <hare/base/timestamp.h>

#include <atomic>
#include <mutex>
#include <vector>

namespace hare {

namespace core {
    class Reactor;
} // namespace core

class Event;
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

    std::unique_ptr<Event> wake_up_event_ { nullptr };
    std::unique_ptr<core::Reactor> reactor_ { nullptr };

    EventList active_events_ {};
    Event* current_active_event_ { nullptr };

    mutable std::mutex mutex_ {};
    std::list<Thread::Task> pending_funcs_ {};

#ifdef HARE_DEBUG
    int64_t cycle_index_ { 0 };
#endif

public:
    Cycle(std::string& reactor_type);
    virtual ~Cycle();

    //!
    //! @brief Time when reactor returns, usually means data arrival.
    inline Timestamp reactorReturnTime() const { return reactor_time_; }

    inline void assertInLoopThread()
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

    void wakeup();
    void updateEvent(Event* event);
    void removeEvent(Event* event);
    bool checkEvent(Event* event);

private:
    void abortNotInLoopThread();
    void doPendingFuncs();

    void printActiveEvents() const;
};

} // namespace hare

#endif // !_HARE_NET_CYCLE_H_