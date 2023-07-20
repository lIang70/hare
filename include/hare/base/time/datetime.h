/**
 * @file hare/base/time/datetime.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with datetime.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_TIME_DATETIME_H_
#define _HARE_BASE_TIME_DATETIME_H_

#include <hare/base/fwd.h>

#include <ctime>

#define HARE_START_YEAR 1900

namespace hare {
namespace time {

    namespace detail {

        HARE_API auto get_julian_day_number(std::int32_t _year, std::int32_t _month, std::int32_t _day) noexcept -> std::int32_t;

    } // namespace detail

    /**
     * Local time in unspecified timezone.
     *   A minute is always 60 seconds, no leap seconds.
     **/
    HARE_CLASS_API
    struct HARE_API date_time {
        std::int32_t year_ { HARE_START_YEAR }; // [1900, 2500]
        std::int32_t month_ { 1 }; // [1, 12]
        std::int32_t day_ { 1 }; // [1, 31]
        std::int32_t hour_ { 0 }; // [0, 23]
        std::int32_t minute_ { 0 }; // [0, 59]
        std::int32_t second_ { 0 }; // [0, 59]

    public:
        date_time();
        explicit date_time(const std::tm&);

        // "yyyy-MM-dd HH:MM:SS"
        auto to_fmt() const -> std::string;

        HARE_INLINE
        auto year() const -> std::int32_t { return year_; }
        HARE_INLINE
        auto month() const -> std::int32_t { return month_; }
        HARE_INLINE
        auto day() const -> std::int32_t { return day_; }
        HARE_INLINE
        auto hour() const -> std::int32_t { return hour_; }
        HARE_INLINE
        auto minute() const -> std::int32_t { return minute_; }
        HARE_INLINE
        auto second() const -> std::int32_t { return second_; }
    };

    HARE_CLASS_API
    class HARE_API date {
        std::int32_t julian_day_number_ { 0 };

    public:
        struct ymd {
            std::int32_t year; // [1900..2500]
            std::int32_t month; // [1..12]
            std::int32_t day; // [1..31]
        };

        static const std::int32_t DAYS_PER_WEEK { 7 };

        date() = default;

        /**
         * @brief Constucts a yyyy-mm-dd Date.
         *
         *   1 <= month <= 12
         **/
        HARE_INLINE
        date(std::int32_t _year, std::int32_t _month, std::int32_t _day)
            : julian_day_number_(detail::get_julian_day_number(_year, _month, _day))
        { }

        HARE_INLINE
        explicit date(std::int32_t _julian_day_number)
            : julian_day_number_(_julian_day_number)
        { }

        HARE_INLINE
        explicit date(const std::tm& _tm)
            : julian_day_number_(
                detail::get_julian_day_number(
                    _tm.tm_year + HARE_START_YEAR,
                    _tm.tm_mon + 1,
                    _tm.tm_mday))
        { }

        // default copy/assignment/dtor are Okay

        void swap(date& _that)
        {
            std::swap(julian_day_number_, _that.julian_day_number_);
        }

        auto valid() const -> bool { return julian_day_number_ > 0; }

        /**
         * @brief Converts to yyyy-mm-dd format.
         **/
        auto to_fmt() const -> std::string;

        auto detail() const -> date::ymd;

        auto year() const -> std::int32_t
        {
            return detail().year;
        }

        auto month() const -> std::int32_t
        {
            return detail().month;
        }

        auto day() const -> std::int32_t
        {
            return detail().day;
        }

        /**
         * @brief [0, 1, ..., 6] => [Sunday, Monday, ..., Saturday ]
         **/
        auto week_day() const -> std::int32_t
        {
            return (julian_day_number_ + 1) % DAYS_PER_WEEK;
        }

        auto julian_day_number() const -> std::int32_t { return julian_day_number_; }
    };

    HARE_API
    HARE_INLINE
    auto operator<(date _date_x, date _date_y) -> bool
    {
        return _date_x.julian_day_number() < _date_y.julian_day_number();
    }

    HARE_API
    HARE_INLINE
    auto operator==(date _date_x, date _date_y) -> bool
    {
        return _date_x.julian_day_number() == _date_y.julian_day_number();
    }

} // namespace time
} // namespace hare

#endif // _HARE_BASE_TIME_DATETIME_H_
