#include <hare/base/time/datetime.h>

namespace hare {
namespace time {

    namespace detail {

        // algorithm and explanation see:
        //   http://www.faqs.org/faqs/calendars/faq/part2/
        static auto get_julian_day_number(int32_t _year, int32_t _month, int32_t _day) noexcept -> int32_t
        {
            static_assert(sizeof(int) >= sizeof(int32_t), "request 32 bit integer at least.");
            auto a = (14 - _month) / 12;
            auto y = _year + 4800 - a;
            auto m = _month + 12 * a - 3;
            return _day + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400 - 32045;
        }

        static auto get_year_month_day(int32_t _julian_day_number) -> date::ymd
        {
            auto a = _julian_day_number + 32044;
            auto b = (4 * a + 3) / 146097;
            auto c = a - ((b * 146097) / 4);
            auto d = (4 * c + 3) / 1461;
            auto e = c - ((1461 * d) / 4);
            auto m = (5 * e + 2) / 153;
            return {
                b * 100 + d - 4800 + (m / 10),
                m + 3 - 12 * (m / 10),
                e - ((153 * m + 2) / 5) + 1
            };
        }

    } // namespace detail

    date_time::date_time(const struct tm& _tm)
        : year_(_tm.tm_year + HARE_START_YEAR)
        , month_(_tm.tm_mon + 1)
        , day_(_tm.tm_mday)
        , hour_(_tm.tm_hour)
        , minute_(_tm.tm_min)
        , second_(_tm.tm_sec)
    {
    }

    auto date_time::to_fmt() const -> std::string
    {
        std::array<char, static_cast<size_t>(HARE_SMALL_FIXED_SIZE) * 2> buffer {};
        auto ret = ::snprintf(buffer.data(), buffer.size(), "%04d-%02d-%02d %02d:%02d:%02d",
            year_, month_, day_, hour_, minute_, second_);
        H_UNUSED(ret);
        return buffer.data();
    }

    const int32_t date::DAYS_PER_WEEK { 7 };
    const int32_t date::JULIAN_DAY_OF_19700101 { detail::get_julian_day_number(1970, 1, 1) };

    date::date(int32_t year, int32_t month, int32_t day)
        : julian_day_number_(detail::get_julian_day_number(year, month, day))
    {
    }

    date::date(const struct tm& _tm)
        : julian_day_number_(
            detail::get_julian_day_number(
                _tm.tm_year + HARE_START_YEAR,
                _tm.tm_mon + 1,
                _tm.tm_mday))
    {
    }

    auto date::to_fmt() const -> std::string
    {
        std::array<char, HARE_SMALL_FIXED_SIZE> buffer {};
        ymd year_month_day(detail());
        auto ret = ::snprintf(buffer.data(), buffer.size(), "%4d-%02d-%02d", year_month_day.year, year_month_day.month, year_month_day.day);
        H_UNUSED(ret);
        return buffer.data();
    }

    auto date::detail() const -> date::ymd
    {
        return detail::get_year_month_day(julian_day_number_);
    }

} // namespace time
} // namespace hare
