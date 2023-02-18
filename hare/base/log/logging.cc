#include "hare/base/log/util.h"
#include "hare/base/thread/local.h"
#include <hare/base/detail/datetime.h>
#include <hare/base/logging.h>

#include <cassert>
#include <cstring>

namespace hare {
namespace log {

    thread_local char t_errno_buf[512];
    thread_local char t_time[64];
    thread_local int64_t t_last_second;

    const char* strErrorno(int errorno)
    {
        return ::strerror_r(errorno, t_errno_buf, sizeof(t_errno_buf));
    }

    static LogLevel initLogLevel()
    {
        if (::getenv("HARE_LOG_TRACE"))
            return LogLevel::TRACE;
        else if (::getenv("HARE_LOG_DEBUG"))
            return LogLevel::DEBUG;
        else
            return LogLevel::INFO;
    }

    const char* LogLevelName[static_cast<int32_t>(LogLevel::NUM_LOG_LEVELS)] = {
        "[TRACE ] ",
        "[DEBUG ] ",
        "[INFO  ] ",
        "[WARN  ] ",
        "[ERROR ] ",
        "[FATAL ] ",
    };

    inline Stream& operator<<(Stream& s, const Logger::FilePath& v)
    {
        s.append(v.data_, v.size_);
        return s;
    }

    void defaultOutput(const char* msg, int len)
    {
        auto n = ::fwrite(msg, 1, len, stdout);
        H_UNUSED(n);
    }

    void defaultFlush()
    {
        ::fflush(stdout);
    }

    TimeZone g_log_time_zone;
    LogLevel g_log_level = initLogLevel();
    Logger::Output g_output = defaultOutput;
    Logger::Flush g_flush = defaultFlush;

}

struct Helper {
    const char* str_;
    const std::size_t len_;

    Helper(const char* str, std::size_t len)
        : str_(str)
        , len_(len)
    {
        assert(strlen(str) == len_);
    }

    friend inline log::Stream& operator<<(log::Stream& s, Helper v)
    {
        s.append(v.str_, v.len_);
        return s;
    }
};

Logger::Data::Data(log::LogLevel level, int old_errno, const FilePath& file, int line)
    : time_(Timestamp::now())
    , stream_()
    , level_(level)
    , line_(line)
    , base_name_(file)
{
    formatTime();
    current_thread::tid();

    stream_ << Helper(current_thread::tidString().c_str(), current_thread::tidString().length());
    stream_ << Helper(log::LogLevelName[static_cast<int32_t>(level)], 9);

    if (old_errno != 0) {
        stream_ << log::strErrorno(old_errno) << " (errno=" << old_errno << ") ";
    }
}

void Logger::Data::formatTime()
{
    auto micro_seconds_since_epoch = time_.microSecondsSinceEpoch();
    auto seconds = time_.secondsSinceEpoch();
    auto micro_seconds = micro_seconds_since_epoch - seconds * Timestamp::MICROSECONDS_PER_SECOND;

    if (seconds != log::t_last_second) {
        log::t_last_second = seconds;
        struct time::DateTime dt;
        if (log::g_log_time_zone) {
            dt = log::g_log_time_zone.toLocalTime(seconds);
        } else {
            dt = TimeZone::toUtcTime(seconds);
        }

        auto len = snprintf(log::t_time, sizeof(log::t_time), "%4d-%02d-%02d %02d:%02d:%02d",
            dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
        assert(len == 19);
        H_UNUSED(len);
    }

    if (log::g_log_time_zone) {
        log::detail::Fmt us(".%06d ", micro_seconds);
        assert(us.length() == 8);
        stream_ << Helper(log::t_time, 19) << Helper(us.data(), 8);
    } else {
        log::detail::Fmt us(".%06dZ ", micro_seconds);
        assert(us.length() == 9);
        stream_ << Helper(log::t_time, 19) << Helper(us.data(), 9);
    }
}

void Logger::Data::finish()
{
    stream_ << " - " << base_name_ << ':' << line_ << '\n';
}

Logger::Logger(FilePath file, int line)
    : d_(log::LogLevel::INFO, 0, file, line)
{
}

Logger::Logger(FilePath file, int line, log::LogLevel level, const char* func)
    : d_(level, 0, file, line)
{
    d_.stream_ << func << ' ';
}

Logger::Logger(FilePath file, int line, log::LogLevel level)
    : d_(level, 0, file, line)
{
}

Logger::Logger(FilePath file, int line, bool abort)
    : d_(abort ? log::LogLevel::FATAL : log::LogLevel::Error, errno, file, line)
{
}

Logger::~Logger()
{
    d_.finish();

    if (d_.level_ >= log::g_log_level) {
        const log::Stream::Buffer& buf(stream().buffer());
        log::g_output(buf.data(), buf.length());
    }

    if (d_.level_ == log::LogLevel::FATAL) {
        log::g_flush();
        std::abort();
    }
}

log::LogLevel Logger::logLevel()
{
    return log::g_log_level;
}

void Logger::setLogLevel(log::LogLevel level)
{
    log::g_log_level = level;
}

void Logger::setOutput(Output output)
{
    log::g_output = output;
}

void Logger::setFlush(Flush flush)
{
    log::g_flush = flush;
}

void Logger::setTimeZone(const TimeZone& tz)
{
    log::g_log_time_zone = tz;
}

}