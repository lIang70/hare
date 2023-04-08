/**
 * @file hare/base/time/time_zone.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with time_zone.h
 * @version 0.1-beta
 * @date 2023-02-09
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _HARE_BASE_TIME_ZONE_H_
#define _HARE_BASE_TIME_ZONE_H_

#include <hare/base/util.h>
#include <hare/base/time/datetime.h>

namespace hare {

class HARE_API TimeZone {
    struct Data;
    Data* d_ { nullptr };

public:
    TimeZone() = default; // an invalid timezone
    TimeZone(int east_of_utc, const char* tz_name); // a fixed timezone
    TimeZone(const TimeZone& another);
    ~TimeZone();

    auto operator=(const TimeZone& another) -> TimeZone&;

    static auto UTC() -> TimeZone;
    // gmtime(3)
    static auto toUtcTime(int64_t seconds_since_epoch) -> time::DateTime;
    // timegm(3)
    static auto fromUtcTime(const time::DateTime&) -> int64_t;

    inline explicit operator bool() const { return d_ != nullptr; }

    auto toLocalTime(int64_t seconds_since_epoch, int* utc_offset = nullptr) const -> time::DateTime;
    auto fromLocalTime(const time::DateTime&, bool post_transition = false) const -> int64_t;

};

} // namespace hare

#endif // !_HARE_BASE_TIME_ZONE_H_