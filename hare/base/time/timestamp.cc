#include <hare/base/time/datetime.h>
#include <hare/base/time/timestamp.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

#include <array>
#include <chrono>

namespace hare {

auto Timestamp::Now() -> Timestamp
{
    return Timestamp(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

auto Timestamp::ToString() const -> std::string
{
    auto seconds = SecondsSinceEpoch();
    auto microseconds = microseconds_since_epoch_ - seconds * HARE_MICROSECONDS_PER_SECOND;
    return fmt::format("{:}.{:06d}", seconds, microseconds);
}

auto Timestamp::ToFmt(bool _show_microseconds) const -> std::string
{
    auto seconds = SecondsSinceEpoch();
    std::tm tm_time { };
#ifdef H_OS_WIN32
    ::gmtime_s(&tm_time, &seconds);
#else
    ::gmtime_r(&seconds, &tm_time);
#endif

    if (_show_microseconds) {
        auto microseconds = static_cast<std::int32_t>(microseconds_since_epoch_ - seconds * HARE_MICROSECONDS_PER_SECOND);
        return fmt::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:06d}",
            tm_time.tm_year + HARE_START_YEAR, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
            microseconds);
    } else {
        return fmt::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}",
            tm_time.tm_year + HARE_START_YEAR, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
}

} // namespace hare