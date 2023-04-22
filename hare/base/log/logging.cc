#include <hare/base/logging.h>

#include "hare/base/thread/local.h"
#include <hare/base/time/datetime.h>
#include <hare/base/util/system_info.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#define EAST_OF_EIGHT (8 * 3600)

namespace hare {
namespace log {

    thread_local std::array<char, static_cast<size_t>(HARE_SMALL_FIXED_SIZE* HARE_SMALL_FIXED_SIZE) / 2> t_errno_buf;
    thread_local std::array<char, static_cast<size_t>(HARE_SMALL_FIXED_SIZE) * 2> t_time;
    thread_local int64_t t_last_second;

    static const std::array<const char*, static_cast<int32_t>(LEVEL_NBRS)> level_name = {
        "[TRACE ] ",
        "[DEBUG ] ",
        "[INFO  ] ",
        "[WARN  ] ",
        "[ERROR ] ",
        "[FATAL ] ",
    };
    const int32_t level_name_len { 9 };

    class helper {
        const char* str_;
        const size_t len_;

    public:
        helper(const char* _str, size_t _len)
            : str_(_str)
            , len_(_len)
        {
            assert(strlen(_str) == len_);
        }

        friend inline auto operator<<(stream& _stream, helper _helper) -> stream&
        {
            _stream.append(_helper.str_, static_cast<int>(_helper.len_));
            return _stream;
        }
    };

    auto errnostr(int _errorno) -> const char*
    {
        ::strerror_r(_errorno, t_errno_buf.data(), t_errno_buf.size());
        return t_errno_buf.data();
    }

    static auto init_level() -> LEVEL
    {
        if (::getenv("HARE_LOG_TRACE") != nullptr) {
            return LEVEL_TRACE;
        }
        if (::getenv("HARE_LOG_DEBUG") != nullptr) {
            return LEVEL_DEBUG;
        }
        return LEVEL_INFO;
    }

    static inline auto operator<<(stream& _stream, const logger::file_path& _file_path) -> stream&
    {
        _stream.append(_file_path.data(), _file_path.size());
        return _stream;
    }

    static inline void default_output(const char* _msg, int _len)
    {
        auto fwrite_n = ::fwrite(_msg, 1, _len, stdout);
        H_UNUSED(fwrite_n);
    }

    static void default_flush()
    {
        ::fflush(stdout);
    }

    timezone g_log_time_zone { EAST_OF_EIGHT, "" };
    LEVEL g_log_level = init_level();
    logger::output g_output = default_output;
    logger::flush g_flush = default_flush;

} // namespace log

logger::data::data(log::LEVEL _level, int _old_errno, const file_path& _file, int _line)
    : time_(timestamp::now())
    , stream_()
    , level_(_level)
    , line_(_line)
    , base_name_(_file)
{
    format_time();
    current_thread::tid();

    stream_ << log::helper(log::level_name[static_cast<int32_t>(_level)], log::level_name_len);
    stream_ << "#(" << log::helper(current_thread::tid_str().c_str(), current_thread::tid_str().length()) << ") ";

    if (_old_errno != 0) {
        stream_ << log::errnostr(_old_errno) << " (errno=" << _old_errno << ") ";
    }
}

void logger::data::format_time()
{
    static const auto s_time_len = 19;
    static const auto s_fmt_len = 8;
    auto micro_seconds_since_epoch = time_.microseconds_since_epoch();
    auto seconds = time_.seconds_since_epoch();
    auto micro_seconds = micro_seconds_since_epoch - seconds * static_cast<int64_t>(MICROSECONDS_PER_SECOND);

    if (seconds != log::t_last_second) {
        log::t_last_second = seconds;
        struct time::date_time tdt;
        if (log::g_log_time_zone) {
            tdt = log::g_log_time_zone.to_local(seconds);
        } else {
            tdt = timezone::to_utc_time(seconds);
        }

        auto len = ::snprintf(log::t_time.data(), log::t_time.size(), "%4d-%02d-%02d %02d:%02d:%02d",
            tdt.year(), tdt.month(), tdt.day(), tdt.hour(), tdt.minute(), tdt.second());
        assert(len == 19);
        H_UNUSED(len);
    }

    if (log::g_log_time_zone) {
        log::detail::fmt fmt_us(".%06d ", micro_seconds);
        assert(fmt_us.length() == 8);
        stream_ << log::helper(log::t_time.data(), s_time_len) << log::helper(fmt_us.data(), s_fmt_len);
    } else {
        log::detail::fmt fmt_us(".%06dZ ", micro_seconds);
        assert(fmt_us.length() == 9);
        stream_ << log::helper(log::t_time.data(), s_time_len) << log::helper(fmt_us.data(), s_fmt_len + 1);
    }
}

void logger::data::finish()
{
    if (level_ <= log::LEVEL_DEBUG) {
        stream_ << " - " << base_name_ << ':' << line_;
    }
    stream_ << '\n';
}

logger::logger(file_path _file, int _line)
    : data_(log::LEVEL_INFO, 0, _file, _line)
{
}

logger::logger(file_path _file, int _line, log::LEVEL _level, const char* _func)
    : data_(_level, 0, _file, _line)
{
    if (log::g_log_level <= log::LEVEL_DEBUG) {
        data_.stream_ << "<" << _func << "> ";
    }
}

logger::logger(file_path _file, int _line, log::LEVEL _level)
    : data_(_level, 0, _file, _line)
{
}

logger::logger(file_path _file, int _line, bool _abort)
    : data_(_abort ? log::LEVEL_FATAL : log::LEVEL_ERROR, errno, _file, _line)
{
}

logger::~logger()
{
    data_.finish();

    if (data_.level_ >= log::g_log_level) {
        const log::stream::cache& buf(stream().buffer());
        log::g_output(buf.begin(), buf.size());
    }

    if (log::g_flush) {
        log::g_flush();
    }

    if (data_.level_ == log::LEVEL_FATAL) {
        ::fwrite(stream().buffer().begin(), 1, stream().buffer().size(), stderr);
        ::fflush(stderr);
        std::abort();
    }
}

auto logger::level() -> log::LEVEL
{
    return log::g_log_level;
}

void logger::set_level(log::LEVEL _level)
{
    log::g_log_level = _level;
}

void logger::set_output(output _output)
{
    log::g_output = std::move(_output);
}

void logger::set_flush(flush _flush)
{
    log::g_flush = std::move(_flush);
}

void logger::set_timezone(const timezone& _time_zone)
{
    log::g_log_time_zone = _time_zone;
}

} // namespace hare