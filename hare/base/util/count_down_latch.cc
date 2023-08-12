#include "hare/base/fwd-inl.h"
#include <hare/base/util/count_down_latch.h>

#include <condition_variable>
#include <mutex>

namespace hare {
namespace util {

    HARE_IMPL_DEFAULT(
        CountDownLatch,
        mutable std::mutex mutex {};
        std::uint32_t count { 0 };
        std::condition_variable cv {};
    )

    CountDownLatch::CountDownLatch(std::uint32_t count)
        : impl_(new CountDownLatchImpl)
    {
        d_ptr(impl_)->count = count;
    }

    CountDownLatch::~CountDownLatch()
    {
        d_ptr(impl_)->cv.notify_all();
        delete impl_;
    }

    void CountDownLatch::CountDown()
    {
        std::lock_guard<std::mutex> lock(d_ptr(impl_)->mutex);
        --d_ptr(impl_)->count;
        if (d_ptr(impl_)->count == 0U) {
            d_ptr(impl_)->cv.notify_all();
        }
    }

    void CountDownLatch::Await(std::int32_t milliseconds)
    {
        std::unique_lock<std::mutex> lock(d_ptr(impl_)->mutex);
        while (d_ptr(impl_)->count > 0) {
            if (milliseconds > 0) {
                d_ptr(impl_)->cv.wait_for(lock, std::chrono::milliseconds(milliseconds));
                break;
            }
            d_ptr(impl_)->cv.wait(lock);
        }
    }

    auto CountDownLatch::Count() -> std::uint32_t
    {
        std::lock_guard<std::mutex> lock(d_ptr(impl_)->mutex);
        return d_ptr(impl_)->count;
    }

} // namespace util
} // namespace hare
