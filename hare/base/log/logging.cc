#include <hare/base/logging.h>

#include "hare/base/log/util.h"
#include "hare/base/util.h"
#include <hare/base/time/datetime.h>
#include <hare/base/util/system_info.h>

#include <cassert>
#include <cstdio>
#include <cstring>

namespace hare {
namespace log {

    thread_local char t_errno_buf[HARE_SMALL_FIXED_SIZE * HARE_SMALL_FIXED_SIZE / 2];
    thread_local char t_time[HARE_SMALL_FIXED_SIZE * 2];
    thread_local int64_t t_last_second;

    auto strErrorno(int errorno) -> const char*
    {
        ::strerror_r(errorno, t_errno_buf, sizeof(t_errno_buf));
        return t_errno_buf;
    }

    static auto initLogLevel() -> Level
    {
        if (::getenv("HARE_LOG_TRACE") != nullptr) {
            return Level::LOG_TRACE;
        }
        if (::getenv("HARE_LOG_DEBUG") != nullptr) {
            return Level::LOG_DEBUG;
        }
        return Level::LOG_INFO;
    }

    const char* LevelName[static_cast<int32_t>(Level::LOG_LEVELS)] = {
        "[TRACE ] ",
        "[DEBUG ] ",
        "[INFO  ] ",
        "[WARN  ] ",
        "[ERROR ] ",
        "[FATAL ] ",
    };
    const int32_t LevelNameLen { 9 };

    inline auto operator<<(Stream& stream, const Logger::FilePath& file_path) -> Stream&
    {
        stream.append(file_path.data(), file_path.size());
        return stream;
    }

    void defaultOutput(const char* msg, int len)
    {
        auto fwrite_n = ::fwrite(msg, 1, len, stdout);
        H_UNUSED(fwrite_n);
    }

    void defaultFlush()
    {
        ::fflush(stdout);
    }

    TimeZone g_log_time_zone;
    Level g_log_level = initLogLevel();
    Logger::Output g_output = defaultOutput;
    Logger::Flush g_flush = defaultFlush;

} // namespace log

struct Helper {
    const char* str_;
    const std::size_t len_;

    Helper(const char* str, std::size_t len)
        : str_(str)
        , len_(len)
    {
        assert(strlen(str) == len_);
    }

    friend inline auto operator<<(log::Stream& stream, Helper helper) -> log::Stream&
    {
        stream.append(helper.str_, static_cast<int>(helper.len_));
        return stream;
    }
};

Logger::Data::Data(log::Level level, int old_errno, const FilePath& file, int line)
    : time_(Timestamp::now())
    , stream_()
    , level_(level)
    , line_(line)
    , base_name_(file)
{
    formatTime();

    stream_ << Helper(log::LevelName[static_cast<int32_t>(level)], log::LevelNameLen);
    stream_ << "#(" << util::pid() << ") ";

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
        struct time::DateTime tdt;
        if (log::g_log_time_zone) {
            tdt = log::g_log_time_zone.toLocalTime(seconds);
        } else {
            tdt = TimeZone::toUtcTime(seconds);
        }

        auto len = snprintf(log::t_time, sizeof(log::t_time), "%4d-%02d-%02d %02d:%02d:%02d",
            tdt.year(), tdt.month(), tdt.day(), tdt.hour(), tdt.minute(), tdt.second());
        assert(len == 19);
        H_UNUSED(len);
    }

    if (log::g_log_time_zone) {
        log::detail::Fmt fmt_us(".%06d ", micro_seconds);
        assert(fmt_us.length() == 8);
        stream_ << Helper(log::t_time, 19) << Helper(fmt_us.data(), 8);
    } else {
        log::detail::Fmt fmt_us(".%06dZ ", micro_seconds);
        assert(fmt_us.length() == 9);
        stream_ << Helper(log::t_time, 19) << Helper(fmt_us.data(), 9);
    }
}

void Logger::Data::finish()
{
#ifndef HARE_DEBUG
    if (level_ <= log::LogLevel::DEBUG)
#endif
    {
        stream_ << " - " << base_name_ << ':' << line_;
    }
    stream_ << '\n';
}

Logger::Logger(FilePath file, int line)
    : d_(log::Level::LOG_INFO, 0, file, line)
{
}

Logger::Logger(FilePath file, int line, log::Level level, const char* func)
    : d_(level, 0, file, line)
{
    if (log::g_log_level <= log::Level::LOG_DEBUG) {
        d_.stream_ << func << ' ';
    }
}

Logger::Logger(FilePath file, int line, log::Level level)
    : d_(level, 0, file, line)
{
}

Logger::Logger(FilePath file, int line, bool abort)
    : d_(abort ? log::Level::LOG_FATAL : log::Level::LOG_ERROR, errno, file, line)
{
}

Logger::~Logger()
{
    d_.finish();

    if (d_.level_ >= log::g_log_level) {
        const log::Stream::Buffer& buf(stream().buffer());
        log::g_output(buf.data(), buf.length());
    }

    if (log::g_flush) {
        log::g_flush();
    }

    if (d_.level_ == log::Level::LOG_FATAL) {
        ::fwrite(stream().buffer().data(), 1, stream().buffer().length(), stderr);
        ::fflush(stderr);
        std::abort();
    }
}

auto Logger::level() -> log::Level
{
    return log::g_log_level;
}

void Logger::setLevel(log::Level level)
{
    log::g_log_level = level;
}

void Logger::setOutput(Output output)
{
    log::g_output = std::move(output);
}

void Logger::setFlush(Flush flush)
{
    log::g_flush = std::move(flush);
}

void Logger::setTimeZone(const TimeZone& time_zone)
{
    log::g_log_time_zone = time_zone;
}

} // namespace hare