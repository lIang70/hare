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
 */

#ifndef _HARE_BASE_LOGGING_H_
#define _HARE_BASE_LOGGING_H_

#include <hare/base/log/stream.h>
#include <hare/base/time/time_zone.h>
#include <hare/base/time/timestamp.h>

#include <functional>

namespace hare {
namespace log {

    /**
     * @brief The enumeration of log level.
     *
     */
    enum Level : int32_t {
        LOG_TRACE,
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR,
        LOG_FATAL,
        LOG_LEVELS
    };

    /**
     * @brief Get the infomation of error number.
     *
     * @param errorno The error number.
     * @return const char* The infomation of error.
     */
    HARE_API auto strErrorno(int errorno) -> const char*;

} // namespace log

/**
 * @brief The interface class of log.
 *
 */
class HARE_API Logger {
public:
    using Output = std::function<void(const char*, int)>;
    using Flush = std::function<void()>;

    class HARE_API FilePath {
        const char* data_ { nullptr };
        int32_t size_ { 0 };

    public:
        template <int Length>
        FilePath(const char (&arr)[Length])
            : data_(arr)
            , size_(Length - 1)
        {
#ifdef M_OS_WIN32
            const char* slash = ::strrchr(data_, '\\'); // builtin function
#else
            const char* slash = ::strrchr(data_, '/'); // builtin function
#endif
            if (slash) {
                data_ = slash + 1;
                size_ -= static_cast<int32_t>(data_ - arr);
            }
        }

        explicit FilePath(const char* file_name)
            : data_(file_name)
        {
#ifdef M_OS_WIN32
            const char* slash = ::strrchr(file_name, '\\');
#else
            const char* slash = ::strrchr(file_name, '/');
#endif
            if (slash != nullptr) {
                data_ = slash + 1;
            }
            size_ = static_cast<int32_t>(strlen(data_));
        }

        inline auto data() const -> const char* { return data_; }
        inline auto size() const -> int32_t { return size_; }
    };

private:
    class HARE_API Data {
        friend class Logger;

    private:
        Timestamp time_ {};
        log::Stream stream_ {};
        log::Level level_ {};
        int line_ {};
        FilePath base_name_;

    public:
        Data(log::Level level, int old_errno, const FilePath& file, int line);

        void formatTime();
        void finish();
    };
    struct Data d_;

public:
    Logger(FilePath file, int line);
    Logger(FilePath file, int line, log::Level level);
    Logger(FilePath file, int line, log::Level level, const char* func);
    Logger(FilePath file, int line, bool to_abort);
    ~Logger();

    auto stream() -> log::Stream& { return d_.stream_; }

    /**
     * @brief Set/Get the global level of log.
     *
     */
    static void setLevel(log::Level level);
    static auto level() -> log::Level;

    /**
     * @brief Set the global output function of log.
     *
     */
    static void setOutput(Output output);

    /**
     * @brief Set the global flush function of log.
     *
     */
    static void setFlush(Flush flush);

    /**
     * @brief Set the global time zone of log.
     *
     */
    static void setTimeZone(const TimeZone& time_zone);
};

} // namespace hare

#define LOG_TRACE() \
    hare::Logger(__FILE__, __LINE__, hare::log::LOG_TRACE, __func__).stream()
#define LOG_DEBUG() \
    hare::Logger(__FILE__, __LINE__, hare::log::LOG_DEBUG, __func__).stream()
#define LOG_INFO() \
    hare::Logger(__FILE__, __LINE__, hare::log::LOG_INFO, __func__).stream()
#define LOG_WARNING() \
    hare::Logger(__FILE__, __LINE__, hare::log::LOG_WARNING, __func__).stream()
#define LOG_ERROR() \
    hare::Logger(__FILE__, __LINE__, hare::log::LOG_ERROR, __func__).stream()
#define LOG_FATAL() \
    hare::Logger(__FILE__, __LINE__, hare::log::LOG_FATAL, __func__).stream()
#define SYS_ERROR() \
    hare::Logger(__FILE__, __LINE__, false).stream()
#define SYS_FATAL() \
    hare::Logger(__FILE__, __LINE__, true).stream()

#ifdef HARE_DEBUG
#define HARE_ASSERT(val, what)                                     \
    do {                                                           \
        if (!(val)) {                                              \
            hare::Logger(__FILE__, __LINE__, true).stream()        \
                << "Condition[" << #val << "] failed. " << (what); \
        }                                                          \
    } while (0)
#else
#define HARE_ASSERT(val, what) H_UNUSED(val), H_UNUSED(what)
#endif

#endif // !_HARE_BASE_LOGGING_H_