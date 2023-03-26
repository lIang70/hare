#ifndef _HARE_BASE_COUNT_DOWN_LATCH_H_
#define _HARE_BASE_COUNT_DOWN_LATCH_H_

#include <hare/base/util/util.h>

#include <condition_variable>
#include <mutex>

namespace hare {
namespace util {

    class CountDownLatch {
        mutable std::mutex mutex_ {};
        uint32_t count_ { 0 };
        std::condition_variable cv_ {};

    public:
        using Ptr = std::shared_ptr<CountDownLatch>;

        explicit CountDownLatch(uint32_t count)
            : count_(count)
        {
        }

        void countDown()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            --count_;
            if (count_ == 0U) {
                cv_.notify_all();
            }
        }

        void await(int32_t milliseconds = 0)
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

        auto count() -> uint32_t
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return count_;
        }
    };

} // namespace util
} // namespace hare

#endif // !_HARE_BASE_COUNT_DOWN_LATCH_H_