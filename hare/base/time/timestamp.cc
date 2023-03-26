#include <hare/base/time/timestamp.h>
#include <hare/base/time/datetime.h>

#include <chrono>
#include <cinttypes>

namespace hare {

auto Timestamp::now() -> Timestamp
{
    return Timestamp(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

auto Timestamp::toString() const -> std::string
{
    char buffer[HARE_SMALL_FIXED_SIZE] { 0 };
    auto seconds = secondsSinceEpoch();
    auto microseconds = microseconds_since_epoch_ - seconds * MICROSECONDS_PER_SECOND;
    snprintf(buffer, HARE_SMALL_FIXED_SIZE, "%" PRId64 ".%.06" PRId64, seconds, microseconds);
    return buffer;
}

auto Timestamp::toFormattedString(bool show_microseconds) const -> std::string
{
    char buffer[HARE_SMALL_FIXED_SIZE * 2] { 0 };
    auto seconds = secondsSinceEpoch();
    struct tm tm_time {};
    ::gmtime_r(&seconds, &tm_time);

    if (show_microseconds) {
        auto microseconds = static_cast<int32_t>(microseconds_since_epoch_ - seconds * MICROSECONDS_PER_SECOND);
        snprintf(buffer, sizeof(buffer), "%4d-%02d-%02d %02d:%02d:%02d.%06d",
                                         tm_time.tm_year + HARE_START_YEAR, tm_time.tm_mon + 1, tm_time.tm_mday,
                                         tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                                         microseconds);
    } else {
        snprintf(buffer, sizeof(buffer), "%4d-%02d-%02d %02d:%02d:%02d",
                                         tm_time.tm_year + HARE_START_YEAR, tm_time.tm_mon + 1, tm_time.tm_mday,
                                         tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);        
    }

    return buffer;
}

} // namespace hare