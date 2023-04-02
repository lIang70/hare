//! 
//! @file hare/base/time/datetime.h
//! @author l1ang70 (gog_017@outlook.com)
//! @brief Describe the class associated with
//!   date time.
//! @version 0.1-beta
//! @date 2023-02-09
//! 
//! @copyright Copyright (c) 2023
//! 

#ifndef _HARE_BASE_TIME_DATETIME_H_
#define _HARE_BASE_TIME_DATETIME_H_

#include <hare/base/util/util.h>

#include <ctime>
#include <cinttypes>
#include <string>

#define HARE_START_YEAR 1900

namespace hare {
namespace time {

    // Local time in unspecified timezone.
    // A minute is always 60 seconds, no leap seconds.
    class HARE_API DateTime {
        int32_t year_ { HARE_START_YEAR }; // [1900, 2500]
        int32_t month_ { 1 }; // [1, 12]
        int32_t day_ { 1 }; // [1, 31]
        int32_t hour_ { 0 }; // [0, 23]
        int32_t minute_ { 0 }; // [0, 59]
        int32_t second_ { 0 }; // [0, 59]

    public:
        DateTime() = default;
        explicit DateTime(const struct tm&);

        // "yyyy-MM-dd HH:MM:SS"
        auto toFmtString() const -> std::string;

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

    class HARE_API Date {
        int32_t julian_day_number_ { 0 };

    public:
        struct YearMonthDay {
            int32_t year; // [1900..2500]
            int32_t month; // [1..12]
            int32_t day; // [1..31]
        };

        static const int32_t DAYS_PER_WEEK;
        static const int32_t JULIAN_DAY_OF_19700101;

        Date() = default;

        //!
        //! Constucts a yyyy-mm-dd Date.
        //!
        //! 1 <= month <= 12
        Date(int32_t year, int32_t month, int32_t day);

        explicit Date(int32_t julian_day_number)
            : julian_day_number_(julian_day_number)
        {
        }

        explicit Date(const struct tm&);

        // default copy/assignment/dtor are Okay

        void swap(Date& that)
        {
            std::swap(julian_day_number_, that.julian_day_number_);
        }

        auto valid() const -> bool { return julian_day_number_ > 0; }

        //! Converts to yyyy-mm-dd format.
        auto toFmtString() const -> std::string;

        auto yearMonthDay() const -> struct YearMonthDay;

        auto year() const -> int32_t
        {
            return yearMonthDay().year;
        }

        auto month() const -> int32_t
        {
            return yearMonthDay().month;
        }

        auto day() const -> int32_t
        {
            return yearMonthDay().day;
        }

        // [0, 1, ..., 6] => [Sunday, Monday, ..., Saturday ]
        auto weekDay() const -> int32_t
        {
            return (julian_day_number_ + 1) % DAYS_PER_WEEK;
        }

        auto julianDayNumber() const -> int32_t { return julian_day_number_; }
    };

    HARE_API inline auto operator<(Date date_x, Date date_y) -> bool
    {
        return date_x.julianDayNumber() < date_y.julianDayNumber();
    }

    HARE_API inline auto operator==(Date date_x, Date date_y) -> bool
    {
        return date_x.julianDayNumber() == date_y.julianDayNumber();
    }

} // namespace time
} // namespace hare

#endif // !_HARE_BASE_TIME_DATETIME_H_