#include <hare/base/util/count_down_latch.h>

namespace hare {
namespace util {

    CountDownLatch::CountDownLatch(uint32_t count)
        : count_(count)
    {
    }

    CountDownLatch::~CountDownLatch()
    {
        cv_.notify_all();
    }

    void CountDownLatch::countDown()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        --count_;
        if (count_ == 0U) {
            cv_.notify_all();
        }
    }

    void CountDownLatch::await(int32_t milliseconds)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (count_ > 0) {
            if (milliseconds > 0) {
                cv_.wait_for(lock, std::chrono::milliseconds(milliseconds));
            } else {
                cv_.wait(lock);
            }
        }
    }

    auto CountDownLatch::count() -> uint32_t
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }

} // namespace util
} // namespace hare