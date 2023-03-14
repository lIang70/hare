#include <hare/base/time/timestamp.h>

#include <chrono>
#include <cinttypes>

namespace hare {

Timestamp Timestamp::now()
{
    return Timestamp(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string Timestamp::toString() const
{
    char buffer[32] { 0 };
    auto seconds = secondsSinceEpoch();
    auto microseconds = microseconds_since_epoch_ - seconds * MICROSECONDS_PER_SECOND;
    snprintf(buffer, 32, "%" PRId64 ".%.06" PRId64, seconds, microseconds);
    return buffer;
}

std::string Timestamp::toFormattedString(bool show_microseconds) const
{
    char buffer[64] { 0 };
    auto seconds = secondsSinceEpoch();
    struct tm tm_time {};
    ::gmtime_r(&seconds, &tm_time);

    if (show_microseconds) {
        auto microseconds = static_cast<int32_t>(microseconds_since_epoch_ - seconds * MICROSECONDS_PER_SECOND);
        snprintf(buffer, sizeof(buffer), "%4d-%02d-%02d %02d:%02d:%02d.%06d",
                                         tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                                         tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                                         microseconds);
    } else {
        snprintf(buffer, sizeof(buffer), "%4d-%02d-%02d %02d:%02d:%02d",
                                         tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                                         tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);        
    }

    return buffer;
}

} // namespace hare