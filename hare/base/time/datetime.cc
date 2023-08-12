#include <hare/base/time/datetime.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

namespace hare {
namespace time {

    namespace detail {

        // algorithm and explanation see:
        //   http://www.faqs.org/faqs/calendars/faq/part2/
        auto JulianDayNumber(std::int32_t _year, std::int32_t _month, std::int32_t _day) noexcept -> std::int32_t
        {
            static_assert(sizeof(int) >= sizeof(std::int32_t), "request 32 bit integer at least.");
            auto a = (14 - _month) / 12;
            auto y = _year + 4800 - a;
            auto m = _month + 12 * a - 3;
            return _day + (153 * m + 2) / 5 + y * 365 + y / 4 - y / 100 + y / 400 - 32045;
        }

        static auto GetYMD(std::int32_t _julian_day_number) -> Date::YMD
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

    DateTime::DateTime() = default;

    DateTime::DateTime(const std::tm& _tm)
        : year(_tm.tm_year + HARE_START_YEAR)
        , month(_tm.tm_mon + 1)
        , day(_tm.tm_mday)
        , hour(_tm.tm_hour)
        , minute(_tm.tm_min)
        , second(_tm.tm_sec)
    {
    }

    auto DateTime::ToFmt() const -> std::string
    {
        return fmt::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}", year, month, day, hour, minute, second);
    }

    auto Date::ToFmt() const -> std::string
    {
        YMD year_month_day(Detail());
        return fmt::format("{:04d}-{:02d}-{:02d}", year_month_day.year, year_month_day.month, year_month_day.day);
    }

    auto Date::Detail() const -> Date::YMD
    {
        return detail::GetYMD(julian_day_number_);
    }

} // namespace time
} // namespace hare
