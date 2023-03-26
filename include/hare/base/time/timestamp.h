#ifndef _HARE_BASE_TIMESTAMP_H_
#define _HARE_BASE_TIMESTAMP_H_

#include <hare/base/util/util.h>

#include <cstdint>
#include <string>

namespace hare {

class HARE_API Timestamp {
    int64_t microseconds_since_epoch_ { -1 };

public:
    using Ptr = std::shared_ptr<Timestamp>;

    static const int64_t MICROSECONDS_PER_SECOND { static_cast<int64_t>(1000) * 1000 };

    //!
    //! Get time of now.
    //!
    static auto now() -> Timestamp;
    static auto invalid() -> Timestamp
    {
        return {};
    }

    static auto fromUnixTime(time_t time) -> Timestamp
    {
        return fromUnixTime(time, 0);
    }

    static auto fromUnixTime(time_t time, int microseconds) -> Timestamp
    {
        return Timestamp(static_cast<int64_t>(time) * MICROSECONDS_PER_SECOND + microseconds);
    }

    static auto difference(Timestamp& high, Timestamp& low) -> double
    {
        int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
        return static_cast<double>(diff) / MICROSECONDS_PER_SECOND;
    }

    Timestamp() = default;
    ~Timestamp() = default;

    explicit Timestamp(int64_t micro_seconds_since_epoch)
        : microseconds_since_epoch_(micro_seconds_since_epoch)
    {
    }

    inline void swap(Timestamp& another) { std::swap(microseconds_since_epoch_, another.microseconds_since_epoch_); }
    inline auto valid() const -> bool { return microseconds_since_epoch_ > 0; }
    inline auto microSecondsSinceEpoch() const -> int64_t { return microseconds_since_epoch_; }
    inline auto secondsSinceEpoch() const -> time_t
    {
        return static_cast<time_t>(microseconds_since_epoch_ / MICROSECONDS_PER_SECOND);
    }

    inline auto operator==(const Timestamp& another) const -> bool { return microseconds_since_epoch_ == another.microseconds_since_epoch_; }
    inline auto operator<(const Timestamp& another) const -> bool { return microseconds_since_epoch_ < another.microseconds_since_epoch_; }

    auto toString() const -> std::string;
    auto toFormattedString(bool show_microseconds = true) const -> std::string;
};

} // namespace hare

#endif // !_HARE_BASE_TIMESTAMP_H_