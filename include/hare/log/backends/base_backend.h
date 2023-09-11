/**
 * @file hare/log/backends/base_backend.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with base_backend.h
 * @version 0.1-beta
 * @date 2023-07-12
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_BACKEND_BASE_H_
#define _HARE_LOG_BACKEND_BASE_H_

#include <hare/base/util/thread_pool.h>
#include <hare/log/details/msg.h>

#include <atomic>
#include <mutex>

namespace hare {
namespace log {

using Policy = hare::util::Policy;

HARE_CLASS_API
class HARE_API Backend {
  level_t level_{LEVEL_INFO};

 public:
  virtual ~Backend() = default;

  virtual void Log(detail::msg_buffer_t& _msg, Level _log_level) = 0;
  virtual void Flush() = 0;

  HARE_INLINE
  auto Check(std::int8_t _msg_level) const -> bool {
    return _msg_level >= level_.load(std::memory_order_relaxed);
  }

  HARE_INLINE
  void set_level(Level _log_level) { level_.store(_log_level); }

  HARE_INLINE
  auto level() const -> Level {
    return static_cast<Level>(level_.load(std::memory_order_relaxed));
  }
};

template <typename Mutex = detail::DummyMutex>
class BaseBackend : public Backend, public util::NonCopyable {
 protected:
  mutable Mutex mutex_{};

 public:
  BaseBackend() = default;

  void Log(detail::msg_buffer_t& _msg, Level _log_level) final {
    std::lock_guard<Mutex> lock(mutex_);
    InnerSinkIt(_msg, _log_level);
  }

  void Flush() final {
    std::lock_guard<Mutex> lock(mutex_);
    InnerFlush();
  }

 protected:
  virtual void InnerSinkIt(detail::msg_buffer_t& _msg, Level _log_level) = 0;
  virtual void InnerFlush() = 0;
};

}  // namespace log
}  // namespace hare

#endif  // _HARE_LOG_BACKEND_BASE_H_