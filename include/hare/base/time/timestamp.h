/**
 * @file hare/base/time/timestamp.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with timestamp.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_TIMESTAMP_H_
#define _HARE_BASE_TIMESTAMP_H_

#include <hare/base/fwd.h>

#define HARE_MICROSECONDS_PER_SECOND (1000000)

namespace hare {

HARE_CLASS_API
class HARE_API Timestamp {
  std::int64_t microseconds_since_epoch_{-1};

 public:
  /**
   * @brief Get time of now.
   *
   */
  static auto Now() -> Timestamp;
  static auto Invalid() -> Timestamp { return {}; }

  static auto FromUnixTime(time_t _time) -> Timestamp {
    return FromUnixTime(_time, 0);
  }

  static auto FromUnixTime(time_t _time, std::int64_t _microseconds)
      -> Timestamp {
    return Timestamp(_time * HARE_MICROSECONDS_PER_SECOND + _microseconds);
  }

  static auto Difference(Timestamp& _high, Timestamp& _low) -> double {
    auto diff =
        _high.microseconds_since_epoch() - _low.microseconds_since_epoch();
    return static_cast<double>(diff) / HARE_MICROSECONDS_PER_SECOND;
  }

  Timestamp() = default;
  ~Timestamp() = default;

  explicit Timestamp(std::int64_t _micro_seconds_since_epoch)
      : microseconds_since_epoch_(_micro_seconds_since_epoch) {}

  HARE_INLINE
  void Swap(Timestamp& _another) {
    std::swap(microseconds_since_epoch_, _another.microseconds_since_epoch_);
  }
  HARE_INLINE
  auto Valid() const -> bool { return microseconds_since_epoch_ > 0; }
  HARE_INLINE
  auto microseconds_since_epoch() const -> std::int64_t {
    return microseconds_since_epoch_;
  }
  HARE_INLINE
  auto SecondsSinceEpoch() const -> time_t {
    return static_cast<time_t>(microseconds_since_epoch_ /
                               HARE_MICROSECONDS_PER_SECOND);
  }

  HARE_INLINE
  auto operator==(const Timestamp& _another) const -> bool {
    return microseconds_since_epoch_ == _another.microseconds_since_epoch_;
  }
  HARE_INLINE
  auto operator<(const Timestamp& _another) const -> bool {
    return microseconds_since_epoch_ < _another.microseconds_since_epoch_;
  }

  auto ToString() const -> std::string;
  auto ToFmt(bool _show_microseconds = true) const -> std::string;
};

}  // namespace hare

#endif  // _HARE_BASE_TIMESTAMP_H_