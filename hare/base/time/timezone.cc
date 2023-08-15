#include "hare/base/fwd-inl.h"
#include <hare/base/time/timezone.h>

#include <algorithm>
#include <vector>

#define HOURS_PER_DAY 24
#define MINUTES_PER_HOUR 60
#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_DAY (HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE)

namespace hare {
namespace detail {

    static const std::int32_t JULIAN_DAY_OF_19700101 { time::detail::JulianDayNumber(1970, 1, 1) };

    HARE_INLINE
    void FillHMS(std::uint32_t _seconds, struct time::DateTime* _sdt)
    {
        auto minutes = static_cast<std::int32_t>(_seconds) / SECONDS_PER_MINUTE;
        _sdt->hour = minutes / MINUTES_PER_HOUR;
        _sdt->minute = minutes - _sdt->hour * MINUTES_PER_HOUR;
        _sdt->second = static_cast<std::int32_t>(_seconds) - minutes * SECONDS_PER_MINUTE;
    }

    auto BreakTime(std::int64_t _time) -> struct time::DateTime
    {
        struct time::DateTime sdt { };
        auto seconds = static_cast<std::int32_t>(_time % static_cast<std::int64_t>(SECONDS_PER_DAY));
        auto days = static_cast<std::int32_t>(_time / static_cast<std::int64_t>(SECONDS_PER_DAY));
        // C++11 rounds towards zero.
        if (seconds < 0) {
            seconds += SECONDS_PER_DAY;
            --days;
        }

        detail::FillHMS(seconds, &sdt);
        time::Date date(days + JULIAN_DAY_OF_19700101);
        time::Date::YMD ymd = date.Detail();
        sdt.year = ymd.year;
        sdt.month = ymd.month;
        sdt.day = ymd.day;

        return sdt;
    }

} // namespace detail

HARE_IMPL_DEFAULT(
    Timezone,
    
    struct CompareUTCTime;
    struct CompareLocalTime;

    struct Transition {
        Transition(std::int64_t _uct, std::int64_t _local, std::int32_t _local_idx)
            : utc_time(_uct)
            , local_time(_local)
            , local_time_idx(_local_idx)
        {
        }

        std::int64_t utc_time;
        std::int64_t local_time; // Shifted Epoch
        std::int32_t local_time_idx;

    };

    struct CompareUTCTime {
        auto operator()(const Transition& _lhs, const Transition& _rhs) const -> bool
        {
            return _lhs.utc_time < _rhs.utc_time;
        }
    };

    struct CompareLocalTime {
        auto operator()(const Transition& _lhs, const Transition& _rhs) const -> bool
        {
            return _lhs.local_time < _rhs.local_time;
        }
    };

    struct LocalTime {
        LocalTime(std::int32_t _offset, bool _dst, std::int32_t _idx)
            : utc_offset(_offset)
            , desig_idx(_idx)
            , is_dst(_dst)
        {
        }

        std::int32_t utc_offset; // East of UTC
        std::int32_t desig_idx;
        bool is_dst;
    };

    std::vector<Transition> transitions {};
    std::vector<LocalTime> local_times {};
    std::string abbreviation {};
    std::string tz_string {};
    bool valid { false };

    void AddLocalTime(std::int32_t _utc_offset, bool _is_dst, std::int32_t _desig_idx)
    {
        local_times.emplace_back(_utc_offset, _is_dst, _desig_idx);
    }

    void AddTransition(std::int64_t _utc_time, std::int32_t _local_time_idx)
    {
        auto& local = local_times.at(_local_time_idx);
        transitions.emplace_back(_utc_time, _utc_time + local.utc_offset, _local_time_idx);
    }

    void Clone(TimezoneImpl* other)
    {
        if (!valid || other == nullptr) {
            return;
        }
        other->abbreviation.assign(abbreviation.begin(), abbreviation.end());
        other->local_times.assign(local_times.begin(), local_times.end());
        other->abbreviation = abbreviation;
        other->tz_string = tz_string;
        other->valid = valid;
    }

    auto FindLocalTime(std::int64_t _utc_time) const -> const LocalTime*;
    auto FindLocalTime(const struct time::DateTime& _tdt, bool _post_transition) const -> const LocalTime*;
)

auto TimezoneImpl::FindLocalTime(std::int64_t _utc_time) const -> const TimezoneImpl::LocalTime*
{
    const LocalTime* local { nullptr };

    // row UTC time             isdst  offset  Local time (PRC)
    //  1  1989-09-16 17:00:00Z   0      8.0   1989-09-17 01:00:00
    //  2  1990-04-14 18:00:00Z   1      9.0   1990-04-15 03:00:00
    //  3  1990-09-15 17:00:00Z   0      8.0   1990-09-16 01:00:00
    //  4  1991-04-13 18:00:00Z   1      9.0   1991-04-14 03:00:00
    //  5  1991-09-14 17:00:00Z   0      8.0   1991-09-15 01:00:00

    // input '1990-06-01 00:00:00Z', std::upper_bound returns row 3,
    // so the input is in range of row 2, offset is 9 hours,
    // local time is 1990-06-01 09:00:00
    if (transitions.empty() || _utc_time < transitions.front().utc_time) {
        // FIXME: should be first non dst time zone
        local = &local_times.front();
    } else {
        Transition sentry(_utc_time, 0, 0);
        auto trans_iter = std::upper_bound(transitions.begin(), transitions.end(), sentry, CompareUTCTime());
        assert(trans_iter != transitions.begin());
        if (trans_iter != transitions.end()) {
            --trans_iter;
            local = &local_times[trans_iter->local_time_idx];
        } else {
            // FIXME: use TZ-env
            local = &local_times[transitions.back().local_time_idx];
        }
    }

    return local;
}

auto TimezoneImpl::FindLocalTime(const struct time::DateTime& _tdt, bool _post_transition) const -> const TimezoneImpl::LocalTime*
{
    const auto local_time = Timezone::FromUtcTime(_tdt);

    if (transitions.empty() || local_time < transitions.front().local_time) {
        // FIXME: should be first non dst time zone
        return &local_times.front();
    }

    Transition sentry(0, local_time, 0);
    auto trans_iter = std::upper_bound(transitions.begin(), transitions.end(), sentry, CompareLocalTime());
    assert(trans_iter != transitions.begin());

    if (trans_iter == transitions.end()) {
        // FIXME: use TZ-env
        return &local_times[transitions.back().local_time_idx];
    }

    auto prior_trans = *(trans_iter - 1);
    auto prior_second = trans_iter->utc_time - 1 + local_times[prior_trans.local_time_idx].utc_offset;

    // row UTC time             isdst  offset  Local time (PRC)     Prior second local time
    //  1  1989-09-16 17:00:00Z   0      8.0   1989-09-17 01:00:00
    //  2  1990-04-14 18:00:00Z   1      9.0   1990-04-15 03:00:00  1990-04-15 01:59:59
    //  3  1990-09-15 17:00:00Z   0      8.0   1990-09-16 01:00:00  1990-09-16 01:59:59
    //  4  1991-04-13 18:00:00Z   1      9.0   1991-04-14 03:00:00  1991-04-14 01:59:59
    //  5  1991-09-14 17:00:00Z   0      8.0   1991-09-15 01:00:00

    // input 1991-04-14 02:30:00, found row 4,
    //  4  1991-04-13 18:00:00Z   1      9.0   1991-04-14 03:00:00  1991-04-14 01:59:59
    if (prior_second < local_time) {
        // it's a skip
        if (_post_transition) {
            return &local_times[trans_iter->local_time_idx];
        }

        return &local_times[prior_trans.local_time_idx];
    }

    // input 1990-09-16 01:30:00, found row 4, looking at row 3
    //  3  1990-09-15 17:00:00Z   0      8.0   1990-09-16 01:00:00  1990-09-16 01:59:59
    --trans_iter;
    if (trans_iter != transitions.begin()) {
        prior_trans = *(trans_iter - 1);
        prior_second = trans_iter->utc_time - 1 + local_times[prior_trans.local_time_idx].utc_offset;
    }
    if (local_time <= prior_second) {
        // it's repeat
        if (_post_transition) {
            return &local_times[trans_iter->local_time_idx];
        }

        return &local_times[prior_trans.local_time_idx];
    }

    // otherwise, it's unique
    return &local_times[trans_iter->local_time_idx];
}

// static
auto Timezone::UTC() -> Timezone
{
    return { 0, "UTC" };
}

Timezone::Timezone()
    : impl_(new TimezoneImpl)
{
}

Timezone::Timezone(std::int32_t east_of_utc, const char* name)
    : Timezone()
{
    IMPL->AddLocalTime(east_of_utc, false, 0);
    IMPL->abbreviation = name;
    IMPL->valid = true;
}

Timezone::~Timezone()
{
    delete impl_;
}

Timezone::Timezone(const Timezone& another)
    : Timezone()
{
    if (another) {
        d_ptr(another.impl_)->Clone(IMPL);
    }
}

auto Timezone::operator=(const Timezone& another) -> Timezone&
{
    if (this == &another) {
        return (*this);
    }

    if (another) {
        d_ptr(another.impl_)->Clone(IMPL);
    }
    return (*this);
}

auto Timezone::ToUtcTime(std::int64_t _seconds_since_epoch) -> struct time::DateTime
{
    return detail::BreakTime(_seconds_since_epoch);
}

auto Timezone::FromUtcTime(const struct time::DateTime& _dt) -> std::int64_t
{
    time::Date date(_dt.year, _dt.month, _dt.day);
    auto seconds_in_day = _dt.hour * (MINUTES_PER_HOUR * SECONDS_PER_MINUTE) + _dt.minute * SECONDS_PER_MINUTE + _dt.second;
    auto days = date.julian_day_number() - detail::JULIAN_DAY_OF_19700101;
    return days * SECONDS_PER_DAY + seconds_in_day;
}

Timezone::operator bool() const
{
    return IMPL->valid;
}

auto Timezone::ToLocal(std::int64_t _seconds, std::int32_t* _utc_offset) const -> struct time::DateTime {
    assert(IMPL->valid);

    struct time::DateTime local_time { };
    const auto* local = IMPL -> FindLocalTime(_seconds);

    if (local != nullptr) {
        local_time = detail::BreakTime(_seconds + local->utc_offset);
        if (_utc_offset != nullptr) {
            *_utc_offset = local->utc_offset;
        }
    }

    return local_time;
}

auto Timezone::FromLocal(const struct time::DateTime& _dt, bool _post_transition) const -> std::int64_t
{
    assert(IMPL->valid);
    const auto* local = IMPL->FindLocalTime(_dt, _post_transition);
    const auto local_seconds = FromUtcTime(_dt);
    if (local != nullptr) {
        return local_seconds - local->utc_offset;
    }
    // fallback as if it's UTC time.
    return local_seconds;
}

} // namespace hare
