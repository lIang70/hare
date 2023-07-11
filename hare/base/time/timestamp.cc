#include <hare/base/time/datetime.h>
#include <hare/base/time/timestamp.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

#include <array>
#include <chrono>

namespace hare {

auto timestamp::now() -> timestamp
{
    return timestamp(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

auto timestamp::to_string() const -> std::string
{
    auto seconds = seconds_since_epoch();
    auto microseconds = microseconds_since_epoch_ - seconds * static_cast<std::int64_t>(HARE_MICROSECONDS_PER_SECOND);
    return fmt::format("{:}.{:6d}", seconds, microseconds);
}

auto timestamp::to_fmt(bool show_microseconds) const -> std::string
{
    auto seconds = seconds_since_epoch();
    struct tm tm_time { };
    ::gmtime_r(&seconds, &tm_time);

    if (show_microseconds) {
        auto microseconds = static_cast<std::int32_t>(microseconds_since_epoch_ - seconds * static_cast<std::int64_t>(HARE_MICROSECONDS_PER_SECOND));
        return fmt::format("{:4d}-{:2d}-{:2d} {:2d}:{:2d}:{:2d}.{:06d}",
            tm_time.tm_year + HARE_START_YEAR, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
            microseconds);
    } else {
        return fmt::format("{:4d}-{:2d}-{:2d} {:2d}:{:2d}:{:2d}",
            tm_time.tm_year + HARE_START_YEAR, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
}

} // namespace hare