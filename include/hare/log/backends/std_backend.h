/**
 * @file hare/log/backends/std_backend.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with std_backend.h
 * @version 0.1-beta
 * @date 2023-07-18
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_STD_BACKEND_H_
#define _HARE_LOG_STD_BACKEND_H_

#include <hare/base/fwd.h>
#include <hare/log/backends/base_backend.h>

namespace hare {
namespace log {

template <typename Mutex>
class STDBackend final : public BaseBackend<Mutex> {
 public:
  using CurrentBackend = STDBackend<Mutex>;

  static auto Instance() -> ::hare::Ptr<CurrentBackend> {
    // not thread-safe
    static ::hare::Ptr<CurrentBackend> static_std_backend{new CurrentBackend};
    return static_std_backend;
  }

 private:
  STDBackend() = default;

  void InnerSinkIt(msg_buffer_t& _msg, Level _log_level) final {
    IgnoreUnused(std::fwrite(_msg.data(), 1, _msg.size(),
                             _log_level <= LEVEL_INFO ? stdout : stderr));
    InnerFlush();
  }

  void InnerFlush() final {
    IgnoreUnused(std::fflush(stderr), std::fflush(stdout));
  }
};

using STDBackendMT = STDBackend<std::mutex>;
using STDBackendST = STDBackend<DummyMutex>;

}  // namespace log
}  // namespace hare

#endif  // _HARE_LOG_FILE_BACKEND_H_