#ifndef _HARE_NET_TIMER_H_
#define _HARE_NET_TIMER_H_

#include <hare/base/thread.h>

namespace hare {
namespace net {

    class HARE_API Timer {
        int64_t ms_timeout_ { -1 };
        Thread::Task task_ {};
        bool persist_ { false };

    public:
        Timer(int64_t ms_timeout, Thread::Task task, bool persist = false);

        inline void setTask(Thread::Task& task) { task_ = task; }
        inline void setTimeout(int64_t ms) { ms_timeout_ = ms; }
        inline int64_t timeout() const { return ms_timeout_; }
        inline bool isPersist() const { return persist_; }

        void run();
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TIMER_H_