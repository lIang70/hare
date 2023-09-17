/**
 * @file hare/base/time/timezone.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with time_zone.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_TIMEZONE_H_
#define _HARE_BASE_TIMEZONE_H_

#include <hare/base/time/datetime.h>

namespace hare {

HARE_CLASS_API
class HARE_API Timezone {
  ::hare::detail::Impl* impl_{};

 public:
  Timezone();  // an invalid timezone
  Timezone(std::int32_t _east_of_utc,
           const char* _tz_name);  // a fixed timezone
  ~Timezone();

  Timezone(const Timezone& _another);
  auto operator=(const Timezone& _another) -> Timezone&;

  static auto UTC() -> Timezone;
  // gmtime(3)
  static auto ToUtcTime(std::int64_t _seconds_since_epoch) -> DateTime;
  // timegm(3)
  static auto FromUtcTime(const DateTime& _dt) -> std::int64_t;

  explicit operator bool() const;

  auto ToLocal(std::int64_t _seconds_since_epoch,
               std::int32_t* _utc_offset = nullptr) const -> DateTime;
  auto FromLocal(const DateTime& _dt, bool _post_transition = false) const
      -> std::int64_t;
};

}  // namespace hare

#endif  // _HARE_BASE_TIMEZONE_H_
