#include "hare/base/fwd-inl.h"
#include <hare/base/util/count_down_latch.h>

#include <condition_variable>
#include <mutex>

namespace hare {
namespace util {

    HARE_IMPL_DEFAULT(count_down_latch,
        mutable std::mutex mutex_ {};
        std::uint32_t count_ { 0 };
        std::condition_variable cv_ {};
    )

    count_down_latch::count_down_latch(std::uint32_t count)
        : impl_(new count_down_latch_impl)
    {
        d_ptr(impl_)->count_ = count;
    }

    count_down_latch::~count_down_latch()
    {
        d_ptr(impl_)->cv_.notify_all();
        delete impl_;
    }

    void count_down_latch::count_down()
    {
        std::lock_guard<std::mutex> lock(d_ptr(impl_)->mutex_);
        --d_ptr(impl_)->count_;
        if (d_ptr(impl_)->count_ == 0U) {
            d_ptr(impl_)->cv_.notify_all();
        }
    }

    void count_down_latch::await(std::int32_t milliseconds)
    {
        std::unique_lock<std::mutex> lock(d_ptr(impl_)->mutex_);
        while (d_ptr(impl_)->count_ > 0) {
            if (milliseconds > 0) {
                d_ptr(impl_)->cv_.wait_for(lock, std::chrono::milliseconds(milliseconds));
                break;
            }
            d_ptr(impl_)->cv_.wait(lock);
        }
    }

    auto count_down_latch::count() -> std::uint32_t
    {
        std::lock_guard<std::mutex> lock(d_ptr(impl_)->mutex_);
        return d_ptr(impl_)->count_;
    }

} // namespace util
} // namespace hare
