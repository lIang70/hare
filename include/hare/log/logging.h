/**
 * @file hare/base/logging.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with logging.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_LOGGING_H_
#define _HARE_LOG_LOGGING_H_

#include <hare/base/exception.h>
#include <hare/log/backends/base_backend.h>

#include <vector>

namespace hare {
namespace log {

    /**
     * @brief The interface class of log.
     **/
    HARE_CLASS_API
    class HARE_API logger : public util::non_copyable {
        using backend_list = std::vector<ptr<backend>>;

        level_t level_ { LEVEL_TRACE };
        level_t flush_level_ { LEVEL_WARNING };
        std::atomic<std::uint64_t> msg_id_ { 0 };
        std::string name_ {};
        log_handler error_handle_ {};
        std::vector<ptr<backend>> backends_ {};

    public:
        HARE_INLINE
        void set_level(LEVEL _level)
        {
            level_.store(_level);
        }

        HARE_INLINE
        auto level() const -> LEVEL
        {
            return static_cast<LEVEL>(level_.load(std::memory_order_relaxed));
        }

        HARE_INLINE
        auto check(LEVEL _level) const -> bool
        {
            return _level >= level_.load(std::memory_order_relaxed);
        }

        HARE_INLINE
        void flush_on(LEVEL _level)
        {
            flush_level_.store(_level);
        }

        HARE_INLINE
        auto name() const -> std::string
        {
            return name_;
        }

        HARE_INLINE
        auto backends() const -> backend_list
        {
            return backends_;
        }

        template <typename... Args>
        HARE_INLINE void log(LEVEL _level, fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            if (!check(_level)) {
                return;
            }

            try {
                details::msg msg(&name_, _level);
                fmt::vformat_to(msg.raw_, _fmt, fmt::make_format_args(_args...));
                sink_it(msg);
            } catch (const hare::exception& e) {
                error_handle_(e.what());
            } catch (...) {
                error_handle_("Unknown exeption in logger");
            }
        }

        template <typename... Args>
        HARE_INLINE void trace(fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            log(LEVEL_TRACE, _fmt, _args...);
        }

        template <typename... Args>
        HARE_INLINE void debug(fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            log(LEVEL_DEBUG, _fmt, _args...);
        }

        template <typename... Args>
        HARE_INLINE void info(fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            log(LEVEL_INFO, _fmt, _args...);
        }

        template <typename... Args>
        HARE_INLINE void warning(fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            log(LEVEL_WARNING, _fmt, _args...);
        }

        template <typename... Args>
        HARE_INLINE void error(fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            log(LEVEL_ERROR, _fmt, _args...);
        }

        template <typename... Args>
        HARE_INLINE void fatal(fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            log(LEVEL_FATAL, _fmt, _args...);
        }

        logger(std::string _unique_name, backend_list _backends);
        logger(std::string _unique_name, ptr<backend> _backend);

        virtual ~logger();

        virtual void flush();

    protected:
        HARE_INLINE
        void incr_msg_id(details::msg& _msg)
        {
            _msg.id_ = msg_id_.fetch_add(1, std::memory_order_relaxed);
        }

        HARE_INLINE
        auto should_flush_on(const details::msg& _msg) ->bool
        {
            const auto flush_level = flush_level_.load(std::memory_order_relaxed);
            return (_msg.level_ >= flush_level) && (_msg.level_ < LEVEL_NBRS);
        }

        virtual void sink_it(details::msg& _msg);
    };

} // namespace log
} // namespace hare

#define LOG_TRACE(logger, format, ...) logger->trace(format, ##__VA_ARGS_)
#define LOG_DEBUG(logger, format, ...) logger->debug(format, ##__VA_ARGS_)
#define LOG_INFO(logger, format, ...) logger->info(format, ##__VA_ARGS_)
#define LOG_WARNING(logger, format, ...) logger->warning(format, ##__VA_ARGS_)
#define LOG_ERROR(logger, format, ...) logger->error(format, ##__VA_ARGS_)
#define LOG_FATAL(logger, format, ...) logger->fatal(format, ##__VA_ARGS_)

#endif // !_HARE_LOG_LOGGING_H_