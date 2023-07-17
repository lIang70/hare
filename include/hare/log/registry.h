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
    class HARE_API registry : util::non_copyable {
        std::mutex mutex_for_loggers_ {};
        std::unordered_map<std::string, ptr<logger>> loggers_ {};

    public:
        static auto instance() -> registry&;

        template <typename Iter>
        HARE_INLINE
        static auto create(const std::string& _unique_name, const Iter& begin, const Iter& end) -> ptr<logger>
        {
            auto tmp = std::make_shared<logger>(_unique_name, begin, end);
            instance().register_logger(tmp);
            return tmp;
        }

        template <typename Iter>
        HARE_INLINE
        static auto create(const std::string& _unique_name, const Iter& begin, const Iter& end, std::size_t _max_msg, std::size_t _thr_n) -> ptr<async_logger>
        {
            auto tmp = std::make_shared<async_logger>(_unique_name, begin, end, _max_msg, _thr_n);
            instance().register_logger(tmp);
            return tmp;
        }

        registry(registry&&) = delete;
        auto operator=(registry&&) -> registry& = delete;

        HARE_INLINE
        void register_logger(const ptr<logger>& _logger)
        {
            std::lock_guard<std::mutex> lock(mutex_for_loggers_);
            auto logger_name = _logger->name();
            assert_if_exists(logger_name);
            loggers_[logger_name] = _logger;
        }

        HARE_INLINE
        auto get(const std::string& logger_name) -> ptr<logger>
        {
            std::lock_guard<std::mutex> lock(mutex_for_loggers_);
            auto found = loggers_.find(logger_name);
            return found == loggers_.end() ? nullptr : found->second;
        }

        HARE_INLINE
        void drop(const std::string& logger_name)
        {
            std::lock_guard<std::mutex> lock(mutex_for_loggers_);
            loggers_.erase(logger_name);
        }

        HARE_INLINE
        void drop_all()
        {
            std::lock_guard<std::mutex> lock(mutex_for_loggers_);
            loggers_.clear();
        }

    private:
        registry() = default;

        void assert_if_exists(const std::string& logger_name);
    };

} // namespace log
} // namespace hare

#endif // _HARE_LOG_REGISTRY_H_