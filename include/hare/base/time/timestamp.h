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
class HARE_API timestamp {
    std::int64_t microseconds_since_epoch_ { -1 };

public:
    /**
     * @brief Get time of now.
     *
     */
    static auto now() -> timestamp;
    static auto invalid() -> timestamp
    {
        return {};
    }

    static auto from_unix_time(time_t _time) -> timestamp
    {
        return from_unix_time(_time, 0);
    }

    static auto from_unix_time(time_t _time, std::int64_t _microseconds) -> timestamp
    {
        return timestamp(_time * HARE_MICROSECONDS_PER_SECOND + _microseconds);
    }

    static auto difference(timestamp& _high, timestamp& _low) -> double
    {
        auto diff = _high.microseconds_since_epoch() - _low.microseconds_since_epoch();
        return static_cast<double>(diff) / HARE_MICROSECONDS_PER_SECOND;
    }

    timestamp() = default;
    ~timestamp() = default;

    explicit timestamp(std::int64_t _micro_seconds_since_epoch)
        : microseconds_since_epoch_(_micro_seconds_since_epoch)
    {
    }

    HARE_INLINE
    void swap(timestamp& _another) { std::swap(microseconds_since_epoch_, _another.microseconds_since_epoch_); }
    HARE_INLINE
    auto valid() const -> bool { return microseconds_since_epoch_ > 0; }
    HARE_INLINE
    auto microseconds_since_epoch() const -> std::int64_t { return microseconds_since_epoch_; }
    HARE_INLINE
    auto seconds_since_epoch() const -> time_t
    {
        return static_cast<time_t>(microseconds_since_epoch_ / HARE_MICROSECONDS_PER_SECOND);
    }

    HARE_INLINE
    auto operator==(const timestamp& _another) const -> bool { return microseconds_since_epoch_ == _another.microseconds_since_epoch_; }
    HARE_INLINE
    auto operator<(const timestamp& _another) const -> bool { return microseconds_since_epoch_ < _another.microseconds_since_epoch_; }

    auto to_string() const -> std::string;
    auto to_fmt(bool show_microseconds = true) const -> std::string;
};

} // namespace hare

#endif // _HARE_BASE_TIMESTAMP_H_