#ifndef _HARE_BASE_TIME_ZONE_H_
#define _HARE_BASE_TIME_ZONE_H_

#include <hare/base/detail/datetime.h>
#include <hare/base/util.h>

#include <cinttypes>
#include <memory>

namespace hare {

class HARE_API TimeZone {
    using DateTime = struct time::DateTime;

    struct Data;
    Data* d_ { nullptr };

public:
    TimeZone() = default; // an invalid timezone
    TimeZone(int east_of_utc, const char* tz_name); // a fixed timezone
    ~TimeZone();

    static TimeZone UTC();
    // gmtime(3)
    static DateTime toUtcTime(int64_t seconds_since_epoch);
    // timegm(3)
    static int64_t fromUtcTime(const DateTime&);

    inline explicit operator bool() const { return d_ != nullptr; }

    DateTime toLocalTime(int64_t seconds_since_epoch, int* utc_offset = nullptr) const;
    int64_t fromLocalTime(const DateTime&, bool post_transition = false) const;

};

} // namespace hare

#endif // !_HARE_BASE_TIME_ZONE_H_