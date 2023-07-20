#include <hare/base/time/datetime.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

namespace hare {
namespace time {

    namespace detail {

        // algorithm and explanation see:
        //   http://www.faqs.org/faqs/calendars/faq/part2/
        auto get_julian_day_number(std::int32_t _year, std::int32_t _month, std::int32_t _day) noexcept -> std::int32_t
        {
            static_assert(sizeof(int) >= sizeof(std::int32_t), "request 32 bit integer at least.");
            auto a = (14 - _month) / 12;
            auto y = _year + 4800 - a;
            auto m = _month + 12 * a - 3;
            return _day + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400 - 32045;
        }

        static auto get_year_month_day(std::int32_t _julian_day_number) -> date::ymd
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

    date_time::date_time() = default;

    date_time::date_time(const std::tm& _tm)
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
        return fmt::format("{:04d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}", year_, month_, day_, hour_, minute_, second_);
    }

    auto date::to_fmt() const -> std::string
    {
        ymd year_month_day(detail());
        return fmt::format("{:04d}-{:02d}-{:02d}", year_month_day.year, year_month_day.month, year_month_day.day);
    }

    auto date::detail() const -> date::ymd
    {
        return detail::get_year_month_day(julian_day_number_);
    }

} // namespace time
} // namespace hare
