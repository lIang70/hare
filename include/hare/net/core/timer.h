#ifndef _HARE_NET_CORE_TIMER_H_
#define _HARE_NET_CORE_TIMER_H_

#include <hare/base/util.h>

#include <functional>

namespace hare {
namespace core {

    class HARE_API Timer {
    public:
        using Task = std::function<void()>;

    private:
        int64_t ms_timeout_ { -1 };
        Task task_ {};
        bool persist_ { false };

    public:
        using Ptr = std::shared_ptr<Timer>;
        using Id = uint64_t;

        Timer(int64_t ms_timeout, Task task, bool persist = false);

        inline void setTask(Task& task) { task_ = task; }
        inline auto task() const -> Task { return task_; }
        inline void setTimeout(int64_t time_ms) { ms_timeout_ = time_ms; }
        inline auto timeout() const -> int64_t { return ms_timeout_; }
        inline auto isPersist() const -> bool { return persist_; }

    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_CORE_TIMER_H_