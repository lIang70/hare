#include <hare/base/util/count_down_latch.h>

#include <condition_variable>
#include <mutex>

#include "base/fwd-inl.h"

namespace hare {

// clang-format off
HARE_IMPL_DEFAULT(CountDownLatch,
  mutable std::mutex mutex{};
  std::uint32_t count{0};
  std::condition_variable cv{};
)
// clang-format on

CountDownLatch::CountDownLatch(std::uint32_t count)
    : impl_(new CountDownLatchImpl) {
  IMPL->count = count;
}

CountDownLatch::~CountDownLatch() {
  IMPL->cv.notify_all();
  delete impl_;
}

void CountDownLatch::CountDown() {
  std::lock_guard<std::mutex> lock(IMPL->mutex);
  --IMPL->count;
  if (IMPL->count == 0U) {
    IMPL->cv.notify_all();
  }
}

void CountDownLatch::Await(std::int32_t milliseconds) {
  std::unique_lock<std::mutex> lock(IMPL->mutex);
  while (IMPL->count > 0) {
    if (milliseconds > 0) {
      IMPL->cv.wait_for(lock, std::chrono::milliseconds(milliseconds));
      break;
    }
    IMPL->cv.wait(lock, [=] { return IMPL->count == 0; });
  }
}

auto CountDownLatch::Count() -> std::uint32_t {
  std::lock_guard<std::mutex> lock(IMPL->mutex);
  return IMPL->count;
}

}  // namespace hare
