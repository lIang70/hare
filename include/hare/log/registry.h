/**
 * @file hare/log/registry.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with registry.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_REGISTRY_H_
#define _HARE_LOG_REGISTRY_H_

#include <hare/base/util/non_copyable.h>
#include <hare/log/async_logger.h>

#include <mutex>
#include <unordered_map>

namespace hare {
namespace log {

HARE_CLASS_API
class HARE_API Registry : public ::hare::NonCopyableNorMovable {
  mutable std::mutex mutex_for_loggers_{};
  std::unordered_map<std::string, Ptr<Logger>> loggers_{};

 public:
  static auto Instance() -> Registry&;

  template <typename Iter>
  HARE_INLINE static auto Create(const std::string& _unique_name,
                                 const Iter& begin, const Iter& end)
      -> Ptr<Logger> {
    auto tmp = std::make_shared<Logger>(_unique_name, begin, end);
    Instance().RegisterLogger(tmp);
    return tmp;
  }

  template <typename Iter>
  HARE_INLINE static auto Create(const std::string& _unique_name,
                                 const Iter& begin, const Iter& end,
                                 std::size_t _max_msg, std::size_t _thr_n)
      -> Ptr<AsyncLogger> {
    auto tmp = std::make_shared<AsyncLogger>(_unique_name, begin, end, _max_msg,
                                             _thr_n);
    Instance().RegisterLogger(tmp);
    return tmp;
  }

  HARE_INLINE
  void RegisterLogger(const Ptr<Logger>& _logger) {
    std::lock_guard<std::mutex> lock(mutex_for_loggers_);
    auto logger_name = _logger->name();
    AssertExists(logger_name);
    loggers_[logger_name] = _logger;
  }

  HARE_INLINE
  auto Get(const std::string& logger_name) -> Ptr<Logger> {
    std::lock_guard<std::mutex> lock(mutex_for_loggers_);
    auto found = loggers_.find(logger_name);
    return found == loggers_.end() ? nullptr : found->second;
  }

  HARE_INLINE
  void Drop(const std::string& logger_name) {
    std::lock_guard<std::mutex> lock(mutex_for_loggers_);
    loggers_.erase(logger_name);
  }

  HARE_INLINE
  void DropAll() {
    std::lock_guard<std::mutex> lock(mutex_for_loggers_);
    loggers_.clear();
  }

 private:
  Registry() = default;

  void AssertExists(const std::string& logger_name);
};

}  // namespace log
}  // namespace hare

#endif  // _HARE_LOG_REGISTRY_H_