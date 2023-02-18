#ifndef _HARE_BASE_TIME_DATETIME_H_
#define _HARE_BASE_TIME_DATETIME_H_

#include <hare/base/util.h>

#include <string>
#include <ctime>
#include <utility>

namespace hare {
namespace time {

    // Local time in unspecified timezone.
    // A minute is always 60 seconds, no leap seconds.
    struct HARE_API DateTime {
        DateTime() = default;
        explicit DateTime(const struct tm&);
        DateTime(int32_t _year, int32_t _month, int32_t _day, int32_t _hour, int32_t _minute, int32_t _second)
            : year(_year)
            , month(_month)
            , day(_day)
            , hour(_hour)
            , minute(_minute)
            , second(_second)
        {
        }

        // "yyyy-MM-dd HH:MM:SS"
        std::string toFmtString() const;

        int32_t year { 1900 }; // [1900, 2500]
        int32_t month { 1 }; // [1, 12]
        int32_t day { 1 }; // [1, 31]
        int32_t hour { 0 }; // [0, 23]
        int32_t minute { 0 }; // [0, 59]
        int32_t second { 0 }; // [0, 59]
    };

    class HARE_API Date {
        int32_t julian_day_number_ { 0 };

    public:
        struct YearMonthDay {
            int32_t year; // [1900..2500]
            int32_t month; // [1..12]
            int32_t day; // [1..31]
        };

        static const int32_t DAYS_PER_WEEK { 7 };
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

        bool valid() const { return julian_day_number_ > 0; }

        //! Converts to yyyy-mm-dd format.
        std::string toFmtString() const;

        struct YearMonthDay yearMonthDay() const;

        int32_t year() const
        {
            return yearMonthDay().year;
        }

        int32_t month() const
        {
            return yearMonthDay().month;
        }

        int32_t day() const
        {
            return yearMonthDay().day;
        }

        // [0, 1, ..., 6] => [Sunday, Monday, ..., Saturday ]
        int32_t weekDay() const
        {
            return (julian_day_number_ + 1) % DAYS_PER_WEEK;
        }

        int32_t julianDayNumber() const { return julian_day_number_; }
    };

    HARE_API inline bool operator<(Date x, Date y)
    {
        return x.julianDayNumber() < y.julianDayNumber();
    }

    HARE_API inline bool operator==(Date x, Date y)
    {
        return x.julianDayNumber() == y.julianDayNumber();
    }

} // namespace time
} // namespace hare

#endif // !_HARE_BASE_TIME_DATETIME_H_