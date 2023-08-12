/**
 * @file hare/log/logging.h
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

    namespace detail {
        HARE_API void HandleLoggerError(std::uint8_t, const std::string& error_msg);
    } // namespace detail

    HARE_CLASS_API
    class HARE_API Logger : public util::NonCopyable {
    public:
        using BackendList = std::vector<Ptr<Backend>>;

    protected:
        level_t level_ { LEVEL_TRACE };
        level_t flush_level_ { LEVEL_WARNING };
        std::atomic<std::uint64_t> msg_id_ { 0 };
        Timezone timezone_ {};
        std::string name_ {};
        LogHandler error_handle_ {};
        BackendList backends_ {};

    public:
        HARE_INLINE
        void set_level(Level _level)
        {
            level_.store(_level);
        }

        HARE_INLINE
        auto level() const -> Level
        {
            return static_cast<Level>(level_.load(std::memory_order_relaxed));
        }

        HARE_INLINE
        auto Check(Level _level) const -> bool
        {
            return _level >= level_.load(std::memory_order_relaxed);
        }

        HARE_INLINE
        void FlushOn(Level _level)
        {
            flush_level_.store(_level);
        }

        HARE_INLINE
        void set_timezone(const Timezone& _tz)
        {
            timezone_ = _tz;
        }

        HARE_INLINE
        auto name() const -> std::string
        {
            return name_;
        }

        HARE_INLINE
        auto backends() const -> BackendList
        {
            return backends_;
        }

        template <typename... Args>
        HARE_INLINE void Log(SourceLoc& _loc, Level _level, fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            if (!Check(_level)) {
                return;
            }

            try {
                details::Msg msg(&name_, &timezone_, _level, _loc);
                fmt::format_to(std::back_inserter(msg.raw_), _fmt, _args...);
                SinkIt(msg);
            } catch (const hare::Exception& e) {
                error_handle_(ERROR_MSG, e.what());
            } catch (const std::exception& e) {
                error_handle_(ERROR_MSG, e.what());
            } catch (...) {
                error_handle_(ERROR_MSG, "Unknown exeption in logger");
            }
        }

        template <typename... Args>
        HARE_INLINE void Trace(SourceLoc&& _loc, fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            Log(_loc, LEVEL_TRACE, _fmt, _args...);
        }

        template <typename... Args>
        HARE_INLINE void Debug(SourceLoc&& _loc, fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            Log(_loc, LEVEL_DEBUG, _fmt, _args...);
        }

        template <typename... Args>
        HARE_INLINE void Info(SourceLoc&& _loc, fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            Log(_loc, LEVEL_INFO, _fmt, _args...);
        }

        template <typename... Args>
        HARE_INLINE void Warning(SourceLoc&& _loc, fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            Log(_loc, LEVEL_WARNING, _fmt, _args...);
        }

        template <typename... Args>
        HARE_INLINE void Error(SourceLoc&& _loc, fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            Log(_loc, LEVEL_ERROR, _fmt, _args...);
        }

        template <typename... Args>
        HARE_INLINE void Fatal(SourceLoc&& _loc, fmt::format_string<Args...> _fmt, const Args&... _args)
        {
            Log(_loc, LEVEL_FATAL, _fmt, _args...);
        }

        template <typename Iter>
        HARE_INLINE
        Logger(std::string _unique_name, const Iter& begin, const Iter& end)
            : name_(std::move(_unique_name))
            , error_handle_(detail::HandleLoggerError)
            , backends_(begin, end) // message counter will start from 1. 0-message id will be reserved for controll messages
        {}

        HARE_INLINE
        Logger(std::string _unique_name, BackendList _backends)
            : Logger(std::move(_unique_name), _backends.begin(), _backends.end())
        {}

        HARE_INLINE
        Logger(std::string _unique_name, Ptr<Backend> _backend)
            : Logger(std::move(_unique_name), BackendList { std::move(_backend) })
        {}

        virtual ~Logger() = default;

        virtual void Flush()
        {
            try {
                for (auto& backend : backends_) {
                    backend->Flush();
                }
            } catch (const hare::Exception& e) {
                error_handle_(ERROR_MSG, e.what());
            } catch (const std::exception& e) {
                error_handle_(ERROR_MSG, e.what());
            } catch (...) {
                error_handle_(ERROR_MSG, "Unknown exeption in logger");
            }
        }

    protected:
        HARE_INLINE
        void IncreaseMsgId(details::Msg& _msg)
        {
            _msg.id_ = msg_id_.fetch_add(1, std::memory_order_relaxed);
        }

        HARE_INLINE
        auto ShouldFlushOn(const details::Msg& _msg) -> bool
        {
            const auto flush_level = flush_level_.load(std::memory_order_relaxed);
            return (_msg.level_ >= flush_level) && (_msg.level_ < LEVEL_NBRS);
        }

        virtual void SinkIt(details::Msg& _msg)
        {
            IncreaseMsgId(_msg);

            details::msg_buffer_t formatted {};
            details::FormatMsg(_msg, formatted);

            for (auto& backend : backends_) {
                if (backend->Check(_msg.level_)) {
                    backend->Log(formatted, static_cast<Level>(_msg.level_));
                }
            }

            if (ShouldFlushOn(_msg)) {
                Flush();
            }
        }
    };

} // namespace log
} // namespace hare

#define LOG_TRACE(logger, format, ...) logger->Trace({ __FILE__, __LINE__, __func__ }, format, ##__VA_ARGS__)
#define LOG_DEBUG(logger, format, ...) logger->Debug({ __FILE__, __LINE__, __func__ }, format, ##__VA_ARGS__)
#define LOG_INFO(logger, format, ...) logger->Info({ __FILE__, __LINE__, __func__ }, format, ##__VA_ARGS__)
#define LOG_WARNING(logger, format, ...) logger->Warning({ __FILE__, __LINE__, __func__ }, format, ##__VA_ARGS__)
#define LOG_ERROR(logger, format, ...) logger->Error({ __FILE__, __LINE__, __func__ }, format, ##__VA_ARGS__)
#define LOG_FATAL(logger, format, ...) logger->Fatal({ __FILE__, __LINE__, __func__ }, format, ##__VA_ARGS__)

#endif // _HARE_LOG_LOGGING_H_
