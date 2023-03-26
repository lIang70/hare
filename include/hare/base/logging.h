#ifndef _HARE_BASE_LOGGING_H_
#define _HARE_BASE_LOGGING_H_

#include <cstddef>
#include <hare/base/log/stream.h>
#include <hare/base/time/time_zone.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/util.h>

#include <array>
#include <cstdint>
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

    HARE_API auto strErrorno(int errorno) -> const char*;

} // namespace log

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
        log::LogLevel level_ {};
        int line_ {};
        FilePath base_name_;

    public:
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

    auto stream() -> log::Stream& { return d_.stream_; }

    static auto logLevel() -> log::LogLevel;
    static void setLogLevel(log::LogLevel level);
    static void setOutput(Output output);
    static void setFlush(Flush flush);
    static void setTimeZone(const TimeZone& time_zone);
};

} // namespace hare

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