#include <hare/base/util/count_down_latch.h>

namespace hare {
namespace util {

    count_down_latch::count_down_latch(uint32_t count)
        : count_(count)
    {
    }

    count_down_latch::~count_down_latch()
    {
        cv_.notify_all();
    }

    void count_down_latch::count_down()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        --count_;
        if (count_ == 0U) {
            cv_.notify_all();
        }
    }

    void count_down_latch::await(int32_t milliseconds)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (count_ > 0) {
            if (milliseconds > 0) {
                cv_.wait_for(lock, std::chrono::milliseconds(milliseconds));
                break;
            }
            cv_.wait(lock);
        }
    }

    auto count_down_latch::count() -> uint32_t
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

} // namespace util
} // namespace hare