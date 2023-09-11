#ifndef _HARE_BASE_IO_LOCAL_H_
#define _HARE_BASE_IO_LOCAL_H_

#include <hare/base/io/event.h>
#include <hare/hare-config.h>

#include <sstream>
#include <thread>

namespace hare {
namespace io {
namespace current_thread {

struct Data {
  // io
  io::Cycle* cycle{nullptr};

  // thread
  std::uint64_t tid{};

  Data() {
    std::ostringstream oss{};
    oss << std::this_thread::get_id();
    tid = std::stoull(oss.str());
  }

  Data(const Data&) = delete;
  Data(Data&&) noexcept = delete;

  Data& operator=(const Data&) = delete;
  Data& operator=(Data&&) = delete;
};

HARE_INLINE
auto ThreadData() -> struct Data& {
  static thread_local struct Data thread_data {};
  return thread_data;
}

}  // namespace current_thread

}  // namespace io
}  // namespace hare

#endif  // _HARE_BASE_IO_LOCAL_H_