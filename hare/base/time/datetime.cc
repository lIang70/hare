#include <hare/base/detail/datetime.h>

namespace hare {
namespace time {

    namespace detail {

        // algorithm and explanation see:
        // http://www.faqs.org/faqs/calendars/faq/part2/
        // http://blog.csdn.net/Solstice

        int32_t getJulianDayNumber(int32_t year, int32_t month, int32_t day)
        {
            static_assert(sizeof(int) >= sizeof(int32_t), "Request 32 bit integer at least.");
            int32_t a = (14 - month) / 12;
            int32_t y = year + 4800 - a;
            int32_t m = month + 12 * a - 3;
            return day + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400 - 32045;
        }

        struct Date::YearMonthDay getYearMonthDay(int32_t julianDayNumber)
        {
            int32_t a = julianDayNumber + 32044;
            int32_t b = (4 * a + 3) / 146097;
            int32_t c = a - ((b * 146097) / 4);
            int32_t d = (4 * c + 3) / 1461;
            int32_t e = c - ((1461 * d) / 4);
            int32_t m = (5 * e + 2) / 153;
            Date::YearMonthDay ymd;
            ymd.day = e - ((153 * m + 2) / 5) + 1;
            ymd.month = m + 3 - 12 * (m / 10);
            ymd.year = b * 100 + d - 4800 + (m / 10);
            return ymd;
        }

    } // namespace detail

    DateTime::DateTime(const struct tm& t)
        : year(t.tm_year + 1900)
        , month(t.tm_mon + 1)
        , day(t.tm_mday)
        , hour(t.tm_hour)
        , minute(t.tm_min)
        , second(t.tm_sec)
    {
    }

    std::string DateTime::toFmtString() const
    {
        char buf[64];
        snprintf(buf, sizeof buf, "%04d-%02d-%02d %02d:%02d:%02d",
            year, month, day, hour, minute, second);
        return buf;
    }

    const int32_t Date::JULIAN_DAY_OF_19700101 = detail::getJulianDayNumber(1970, 1, 1);

    Date::Date(int32_t y, int32_t m, int32_t d)
        : julian_day_number_(detail::getJulianDayNumber(y, m, d))
    {
    }

    Date::Date(const struct tm& t)
        : julian_day_number_(
            detail::getJulianDayNumber(
                t.tm_year + 1900,
                t.tm_mon + 1,
                t.tm_mday))
    {
    }

    std::string Date::toFmtString() const
    {
        char buf[32];
        YearMonthDay ymd(yearMonthDay());
        snprintf(buf, sizeof(buf), "%4d-%02d-%02d", ymd.year, ymd.month, ymd.day);
        return buf;
    }

    Date::YearMonthDay Date::yearMonthDay() const
    {
        return detail::getYearMonthDay(julian_day_number_);
    }

} // namespace time
} // namespace hare
