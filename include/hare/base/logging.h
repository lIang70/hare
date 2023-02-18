#ifndef _HARE_BASE_LOGGING_H_
#define _HARE_BASE_LOGGING_H_

#include <hare/base/detail/log_stream.h>
#include <hare/base/time_zone.h>
#include <hare/base/timestamp.h>
#include <hare/base/util.h>

#include <functional>

namespace hare {
namespace log {

    enum class LogLevel : int32_t {
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        Error,
        FATAL,
        NUM_LOG_LEVELS
    };

    HARE_API const char* strErrorno(int errorno);

}

class HARE_API Logger {
public:
    using Output = std::function<void(const char*, int)>;
    using Flush = std::function<void()>;

    struct HARE_API FilePath {
        const char* data_ { nullptr };
        int32_t size_ { 0 };

        template <int Length>
        FilePath(const char (&arr)[Length])
            : data_(arr)
            , size_(Length - 1)
        {
#ifdef M_OS_WIN32
            const char* slash = strrchr(data_, '\\'); // builtin function
#else
            const char* slash = strrchr(data_, '/'); // builtin function
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
            const char* slash = strrchr(file_name, '\\');
#else
            const char* slash = strrchr(file_name, '/');
#endif
            if (slash) {
                data_ = slash + 1;
            }
            size_ = static_cast<int32_t>(strlen(data_));
        }
    };

private:
    struct HARE_API Data {
        Timestamp time_ {};
        log::Stream stream_ {};
        log::LogLevel level_ {};
        int line_ {};
        FilePath base_name_;

        Data(log::LogLevel level, int old_errno, const FilePath& file, int line);

        void formatTime();
        void finish();
    };
    struct Data d_;

public:
    Logger(FilePath file, int line);
    Logger(FilePath file, int line, log::LogLevel level);
    Logger(FilePath file, int line, log::LogLevel level, const char* func);
    Logger(FilePath file, int line, bool to_abort);
    ~Logger();

    log::Stream& stream() { return d_.stream_; }

    static log::LogLevel logLevel();
    static void setLogLevel(log::LogLevel level);
    static void setOutput(Output output);
    static void setFlush(Flush flush);
    static void setTimeZone(const TimeZone& tz);
};

}

#define LOG_TRACE() \
    hare::Logger(__FILE__, __LINE__, hare::log::LogLevel::TRACE, __func__).stream()
#define LOG_DEBUG() \
    hare::Logger(__FILE__, __LINE__, hare::log::LogLevel::DEBUG, __func__).stream()
#define LOG_INFO() \
    hare::Logger(__FILE__, __LINE__, hare::log::LogLevel::INFO, __func__).stream()
#define LOG_WARNING() \
    hare::Logger(__FILE__, __LINE__, hare::log::LogLevel::WARNING, __func__).stream()
#define LOG_ERROR() \
    hare::Logger(__FILE__, __LINE__, hare::log::LogLevel::Error, __func__).stream()
#define LOG_FATAL() \
    hare::Logger(__FILE__, __LINE__, hare::log::LogLevel::FATAL, __func__).stream()
#define SYS_ERROR() \
    hare::Logger(__FILE__, __LINE__, false).stream()
#define SYS_FATAL() \
    hare::Logger(__FILE__, __LINE__, true).stream()

#ifdef HARE_DEBUG
#define HARE_ASSERT(val, what)                                  \
    do {                                                        \
        if (!(val))                                             \
            hare::Logger(__FILE__, __LINE__, true).stream()     \
                << "Condition " << #val << " failed. " << what; \
    } while (0)
#else
#define HARE_ASSERT(val, what) H_UNUSED(val), H_UNUSED(what)
#endif

#endif // !_HARE_BASE_LOGGING_H_