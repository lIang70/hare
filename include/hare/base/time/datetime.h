/**
 * @file hare/base/time/datetime.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with datetime.h
 * @version 0.1-beta
 * @date 2023-02-09
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _HARE_BASE_TIME_DATETIME_H_
#define _HARE_BASE_TIME_DATETIME_H_

#include <hare/base/util.h>

#include <ctime>
#include <string>

#define HARE_START_YEAR 1900

namespace hare {
namespace time {

    // Local time in unspecified timezone.
    // A minute is always 60 seconds, no leap seconds.
    class HARE_API date_time {
        int32_t year_ { HARE_START_YEAR }; // [1900, 2500]
        int32_t month_ { 1 }; // [1, 12]
        int32_t day_ { 1 }; // [1, 31]
        int32_t hour_ { 0 }; // [0, 23]
        int32_t minute_ { 0 }; // [0, 59]
        int32_t second_ { 0 }; // [0, 59]

    public:
        date_time() = default;
        explicit date_time(const struct tm&);

        // "yyyy-MM-dd HH:MM:SS"
        auto to_fmt() const -> std::string;

        inline auto year() -> int32_t& { return year_; }
        inline auto year() const -> int32_t { return year_; }
        inline auto month() -> int32_t& { return month_; }
        inline auto month() const -> int32_t { return month_; }
        inline auto day() -> int32_t& { return day_; }
        inline auto day() const -> int32_t { return day_; }
        inline auto hour() -> int32_t& { return hour_; }
        inline auto hour() const -> int32_t { return hour_; }
        inline auto minute() -> int32_t& { return minute_; }
        inline auto minute() const -> int32_t { return minute_; }
        inline auto second() -> int32_t& { return second_; }
        inline auto second() const -> int32_t { return second_; }
    };

    class HARE_API date {
        int32_t julian_day_number_ { 0 };

    public:
        struct ymd {
            int32_t year; // [1900..2500]
            int32_t month; // [1..12]
            int32_t day; // [1..31]
        };

        static const int32_t DAYS_PER_WEEK;
        static const int32_t JULIAN_DAY_OF_19700101;

        date() = default;

        //!
        //! Constucts a yyyy-mm-dd Date.
        //!
        //! 1 <= month <= 12
        date(int32_t _year, int32_t _month, int32_t _day);

        explicit date(int32_t _julian_day_number)
            : julian_day_number_(_julian_day_number)
        {
        }

        explicit date(const struct tm&);

        // default copy/assignment/dtor are Okay

        void swap(date& _that)
        {
            std::swap(julian_day_number_, _that.julian_day_number_);
        }

        auto valid() const -> bool { return julian_day_number_ > 0; }

        //! Converts to yyyy-mm-dd format.
        auto to_fmt() const -> std::string;

        auto detail() const -> struct ymd;

        auto year() const -> int32_t
        {
            return detail().year;
        }

        auto month() const -> int32_t
        {
            return detail().month;
        }

        auto day() const -> int32_t
        {
            return detail().day;
        }

        // [0, 1, ..., 6] => [Sunday, Monday, ..., Saturday ]
        auto week_day() const -> int32_t
        {
            return (julian_day_number_ + 1) % DAYS_PER_WEEK;
        }

        auto julian_day_number() const -> int32_t { return julian_day_number_; }
    };

    HARE_API inline auto operator<(date _date_x, date _date_y) -> bool
    {
        return _date_x.julian_day_number() < _date_y.julian_day_number();
    }

    HARE_API inline auto operator==(date _date_x, date _date_y) -> bool
    {
        return _date_x.julian_day_number() == _date_y.julian_day_number();
    }

} // namespace time
} // namespace hare

#endif // !_HARE_BASE_TIME_DATETIME_H_