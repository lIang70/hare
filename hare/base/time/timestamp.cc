#include <hare/base/time/datetime.h>
#include <hare/base/time/timestamp.h>

#include <array>
#include <chrono>

namespace hare {

auto timestamp::now() -> timestamp
{
    return timestamp(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

auto timestamp::to_string() const -> std::string
{
    std::array<char, HARE_SMALL_FIXED_SIZE> buffer {};
    auto seconds = seconds_since_epoch();
    auto microseconds = microseconds_since_epoch_ - seconds * static_cast<int64_t>(MICROSECONDS_PER_SECOND);
    auto ret = ::snprintf(buffer.data(), HARE_SMALL_FIXED_SIZE, "%" PRId64 ".%.06" PRId64, seconds, microseconds);
    H_UNUSED(ret);
    return buffer.data();
}

auto timestamp::to_fmt(bool show_microseconds) const -> std::string
{
    std::array<char, static_cast<size_t>(HARE_SMALL_FIXED_SIZE) * 2> buffer {};
    auto seconds = seconds_since_epoch();
    struct tm tm_time { };
    ::gmtime_r(&seconds, &tm_time);

    if (show_microseconds) {
        auto microseconds = static_cast<int32_t>(microseconds_since_epoch_ - seconds * static_cast<int64_t>(MICROSECONDS_PER_SECOND));
        auto ret = ::snprintf(buffer.data(), buffer.size(), "%4d-%02d-%02d %02d:%02d:%02d.%06d",
            tm_time.tm_year + HARE_START_YEAR, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
            microseconds);
            H_UNUSED(ret);
    } else {
        auto ret = ::snprintf(buffer.data(), buffer.size(), "%4d-%02d-%02d %02d:%02d:%02d",
            tm_time.tm_year + HARE_START_YEAR, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
            H_UNUSED(ret);
    }

    return buffer.data();
}

} // namespace hare