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

namespace time_inner {

HARE_API auto JulianDayNumber(std::int32_t _year, std::int32_t _month,
                              std::int32_t _day) noexcept -> std::int32_t;

}  // namespace time_inner

/**
 * Local time in unspecified timezone.
 *   A minute is always 60 seconds, no leap seconds.
 **/
HARE_CLASS_API
struct HARE_API DateTime {
  std::int32_t year{HARE_START_YEAR};  // [1900, 2500]
  std::int32_t month{1};               // [1, 12]
  std::int32_t day{1};                 // [1, 31]
  std::int32_t hour{0};                // [0, 23]
  std::int32_t minute{0};              // [0, 59]
  std::int32_t second{0};              // [0, 59]

  DateTime();
  explicit DateTime(const std::tm&);

  // "yyyy-MM-dd HH:MM:SS"
  auto ToFmt() const -> std::string;
};

HARE_CLASS_API
class HARE_API Date {
  std::int32_t julian_day_number_{0};

 public:
  struct YMD {
    std::int32_t year;   // [1900..2500]
    std::int32_t month;  // [1..12]
    std::int32_t day;    // [1..31]
  };

  static const std::int32_t DAYS_PER_WEEK{7};

  Date() = default;

  /**
   * @brief Constucts a yyyy-mm-dd Date.
   *
   *   1 <= month <= 12
   **/
  HARE_INLINE
  Date(std::int32_t _year, std::int32_t _month, std::int32_t _day)
      : julian_day_number_(time_inner::JulianDayNumber(_year, _month, _day)) {}

  HARE_INLINE
  explicit Date(std::int32_t _julian_day_number)
      : julian_day_number_(_julian_day_number) {}

  HARE_INLINE
  explicit Date(const std::tm& _tm)
      : julian_day_number_(time_inner::JulianDayNumber(
            _tm.tm_year + HARE_START_YEAR, _tm.tm_mon + 1, _tm.tm_mday)) {}

  // default copy/assignment/dtor are Okay

  void Swap(Date& _that) {
    std::swap(julian_day_number_, _that.julian_day_number_);
  }

  auto Valid() const -> bool { return julian_day_number_ >= 0; }

  /**
   * @brief Converts to yyyy-mm-dd format.
   **/
  auto ToFmt() const -> std::string;

  auto Detail() const -> Date::YMD;

  auto Year() const -> std::int32_t { return Detail().year; }

  auto Month() const -> std::int32_t { return Detail().month; }

  auto Day() const -> std::int32_t { return Detail().day; }

  /**
   * @brief [0, 1, ..., 6] => [Sunday, Monday, ..., Saturday ]
   **/
  HARE_INLINE
  auto WeekDay() const -> std::int32_t {
    return (julian_day_number_ + 1) % DAYS_PER_WEEK;
  }

  auto julian_day_number() const -> std::int32_t { return julian_day_number_; }
};

HARE_API
HARE_INLINE
auto operator<(Date _date_x, Date _date_y) -> bool {
  return _date_x.julian_day_number() < _date_y.julian_day_number();
}

HARE_API
HARE_INLINE
auto operator==(Date _date_x, Date _date_y) -> bool {
  return _date_x.julian_day_number() == _date_y.julian_day_number();
}

}  // namespace hare

#endif  // _HARE_BASE_TIME_DATETIME_H_
