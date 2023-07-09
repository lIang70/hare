/**
 * @file hare/base/time/time_zone.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with time_zone.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_TIME_ZONE_H_
#define _HARE_BASE_TIME_ZONE_H_

#include <hare/base/time/datetime.h>

namespace hare {

class HARE_API timezone {
    struct zone_data;
    uptr<zone_data> data_ {};

public:
    timezone() = default; // an invalid timezone
    timezone(int _east_of_utc, const char* _tz_name); // a fixed timezone
    timezone(const timezone& _another);
    ~timezone() = default;;

    auto operator=(const timezone& _another) -> timezone&;

    static auto utc() -> timezone;
    // gmtime(3)
    static auto to_utc_time(int64_t _seconds_since_epoch) -> time::date_time;
    // timegm(3)
    static auto from_utc_time(const time::date_time& _dt) -> int64_t;

    inline explicit operator bool() const { return bool(data_); }

    auto to_local(int64_t _seconds_since_epoch, int* _utc_offset = nullptr) const -> time::date_time;
    auto from_local(const time::date_time& _dt, bool _post_transition = false) const -> int64_t;
};

} // namespace hare

#endif // !_HARE_BASE_TIME_ZONE_H_