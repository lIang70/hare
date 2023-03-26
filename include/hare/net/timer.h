#ifndef _HARE_NET_TIMER_H_
#define _HARE_NET_TIMER_H_

#include <hare/base/util/thread.h>

namespace hare {
namespace net {

    class HARE_API Timer {
        int64_t ms_timeout_ { -1 };
        Thread::Task task_ {};
        bool persist_ { false };

    public:
        Timer(int64_t ms_timeout, Thread::Task task, bool persist = false);

        inline void setTask(Thread::Task& task) { task_ = task; }
        inline void setTimeout(int64_t time_ms) { ms_timeout_ = time_ms; }
        inline auto timeout() const -> int64_t { return ms_timeout_; }
        inline auto isPersist() const -> bool { return persist_; }

        inline auto task() const -> Thread::Task { return task_; }

    };

    using TimerId = uint64_t;

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TIMER_H_