#ifndef _HARE_BASE_TIMESTAMP_H_
#define _HARE_BASE_TIMESTAMP_H_

#include <hare/base/util.h>

#include <cstdint>
#include <string>
#include <utility>

namespace hare {

class HARE_API Timestamp {
    int64_t microseconds_since_epoch_ { -1 };

public:
    static const int64_t MICROSECONDS_PER_SECOND { 1000 * 1000 };

    //!
    //! Get time of now.
    //!
    static Timestamp now();
    static Timestamp invalid()
    {
        return Timestamp();
    }

    static Timestamp fromUnixTime(time_t t)
    {
        return fromUnixTime(t, 0);
    }

    static Timestamp fromUnixTime(time_t t, int microseconds)
    {
        return Timestamp(static_cast<int64_t>(t) * MICROSECONDS_PER_SECOND + microseconds);
    }

    static double difference(Timestamp& high, Timestamp& low)
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

    inline void swap(Timestamp& ts) { std::swap(microseconds_since_epoch_, ts.microseconds_since_epoch_); }
    inline bool valid() const { return microseconds_since_epoch_ > 0; }
    inline int64_t microSecondsSinceEpoch() const { return microseconds_since_epoch_; }
    inline time_t secondsSinceEpoch() const
    {
        return static_cast<time_t>(microseconds_since_epoch_ / MICROSECONDS_PER_SECOND);
    }

    inline bool operator==(const Timestamp& ts) const { return microseconds_since_epoch_ == ts.microseconds_since_epoch_; }
    inline bool operator<(const Timestamp& ts) const { return microseconds_since_epoch_ < ts.microseconds_since_epoch_; }

    std::string toString() const;
    std::string toFormattedString(bool show_microseconds = true) const;
};

} // namespace hare

#endif // !_HARE_BASE_TIMESTAMP_H_