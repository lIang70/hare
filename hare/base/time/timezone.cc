#include <hare/base/time/timezone.h>

// c header
#include <cassert>

// c++ header
#include <algorithm>
#include <string>
#include <vector>

#define HOURS_PER_DAY 24
#define MINUTES_PER_HOUR 60
#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_DAY (HOURS_PER_DAY * MINUTES_PER_HOUR * SECONDS_PER_MINUTE)

namespace hare {
namespace detail {

    static inline void fill_hms(uint32_t _seconds, struct time::date_time* _sdt)
    {
        auto minutes = static_cast<int32_t>(_seconds) / SECONDS_PER_MINUTE;
        _sdt->hour() = minutes / MINUTES_PER_HOUR;
        _sdt->minute() = minutes - _sdt->hour() * MINUTES_PER_HOUR;
        _sdt->second() = static_cast<int32_t>(_seconds) - minutes * SECONDS_PER_MINUTE;
    }

    auto break_time(int64_t _time) -> time::date_time
    {
        struct time::date_time sdt;
        auto seconds = static_cast<int32_t>(_time % static_cast<int64_t>(SECONDS_PER_DAY));
        auto days = static_cast<int32_t>(_time / static_cast<int64_t>(SECONDS_PER_DAY));
        // C++11 rounds towards zero.
        if (seconds < 0) {
            seconds += SECONDS_PER_DAY;
            --days;
        }

        detail::fill_hms(seconds, &sdt);
        time::date date(days + time::date::JULIAN_DAY_OF_19700101);
        time::date::ymd ymd = date.detail();
        sdt.year() = ymd.year;
        sdt.month() = ymd.month;
        sdt.day() = ymd.day;

        return sdt;
    }

} // namespace detail

struct timezone::zone_data {
    struct compare_utc_time;
    struct compare_local_time;

    struct transition {
        transition(int64_t _uct, int64_t _local, int32_t _local_idx)
            : utc_time(_uct)
            , local_time(_local)
            , local_time_idx(_local_idx)
        {
        }

    private:
        int64_t utc_time;
        int64_t local_time; // Shifted Epoch
        int32_t local_time_idx;

        friend struct compare_utc_time;
        friend struct compare_local_time;
        friend struct timezone::zone_data;
    };

    struct compare_utc_time {
        auto operator()(const transition& _lhs, const transition& _rhs) const -> bool
        {
            return _lhs.utc_time < _rhs.utc_time;
        }
    };

    struct compare_local_time {
        auto operator()(const transition& _lhs, const transition& _rhs) const -> bool
        {
            return _lhs.local_time < _rhs.local_time;
        }
    };

    struct local_time {
        local_time(int32_t _offset, bool _dst, int32_t _idx)
            : utc_offset(_offset)
            , desig_idx(_idx)
            , is_dst(_dst)
        {
        }

    private:
        int32_t utc_offset; // East of UTC
        int32_t desig_idx;
        bool is_dst;

        friend class timezone;
        friend struct timezone::zone_data;
    };

    void add_local_time(int32_t _utc_offset, bool _is_dst, int32_t _desig_idx)
    {
        local_times.emplace_back(_utc_offset, _is_dst, _desig_idx);
    }

    void add_transition(int64_t _utc_time, int32_t _local_time_idx)
    {
        auto& local = local_times.at(_local_time_idx);
        transitions.emplace_back(_utc_time, _utc_time + local.utc_offset, _local_time_idx);
    }

    auto clone() const -> uptr<zone_data>
    {
        uptr<zone_data> clone_data(new zone_data);
        clone_data->abbreviation.assign(abbreviation.begin(), abbreviation.end());
        clone_data->local_times.assign(local_times.begin(), local_times.end());
        clone_data->abbreviation = abbreviation;
        clone_data->tz_string = tz_string;
        return std::move(clone_data);
    }

    auto find_local_time(int64_t _utc_time) const -> const local_time*;
    auto find_local_time(const struct time::date_time& _tdt, bool _post_transition) const -> const local_time*;

private:
    std::vector<transition> transitions {};
    std::vector<local_time> local_times {};
    std::string abbreviation {};
    std::string tz_string {};

    friend class timezone;
};

auto timezone::zone_data::find_local_time(int64_t _utc_time) const -> const timezone::zone_data::local_time*
{
    const local_time* local { nullptr };

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
        transition sentry(_utc_time, 0, 0);
        auto trans_iter = std::upper_bound(transitions.begin(), transitions.end(), sentry, compare_utc_time());
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

auto timezone::zone_data::find_local_time(const struct time::date_time& _tdt, bool _post_transition) const -> const timezone::zone_data::local_time*
{
    const auto local_time = from_utc_time(_tdt);

    if (transitions.empty() || local_time < transitions.front().local_time) {
        // FIXME: should be first non dst time zone
        return &local_times.front();
    }

    transition sentry(0, local_time, 0);
    auto trans_iter = std::upper_bound(transitions.begin(), transitions.end(), sentry, compare_local_time());
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
        // printf("SKIP: prev %ld local %ld start %ld\n", prior_second, localtime, transI->localtime);
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
        // printf("REPEAT: prev %ld local %ld start %ld\n", prior_second, localtime, transI->localtime);
        if (_post_transition) {
            return &local_times[trans_iter->local_time_idx];
        }

        return &local_times[prior_trans.local_time_idx];
    }

    // otherwise, it's unique
    return &local_times[trans_iter->local_time_idx];
}

// static
auto timezone::utc() -> timezone
{
    return { 0, "UTC" };
}

timezone::timezone(int32_t east_of_utc, const char* name)
    : data_(new zone_data)
{
    data_->add_local_time(east_of_utc, false, 0);
    data_->abbreviation = name;
}

timezone::timezone(const timezone& another)
{
    if (another) {
        data_ = another.data_->clone();
    }
}

auto timezone::operator=(const timezone& another) -> timezone&
{
    if (this == &another) {
        return (*this);
    }

    if (another) {
        data_ = another.data_->clone();
    }
    return (*this);
}

auto timezone::to_local(int64_t _seconds, int32_t* _utc_offset) const -> struct time::date_time {
    struct time::date_time localTime;
    assert(data_ != nullptr);

    const auto* local = data_->find_local_time(_seconds);

    if (local != nullptr) {
        localTime = detail::break_time(_seconds + local->utc_offset);
        if (_utc_offset != nullptr) {
            *_utc_offset = local->utc_offset;
        }
    }

    return localTime;
}

auto timezone::from_local(const struct time::date_time& _dt, bool _post_transition) const -> int64_t
{
    assert(data_ != nullptr);
    const auto* local = data_->find_local_time(_dt, _post_transition);
    const auto local_seconds = from_utc_time(_dt);
    if (local != nullptr) {
        return local_seconds - local->utc_offset;
    }
    // fallback as if it's UTC time.
    return local_seconds;
}

auto timezone::to_utc_time(int64_t _seconds_since_epoch) -> time::date_time
{
    return detail::break_time(_seconds_since_epoch);
}

auto timezone::from_utc_time(const time::date_time& _dt) -> int64_t
{
    time::date date(_dt.year(), _dt.month(), _dt.day());
    auto seconds_in_day = _dt.hour() * (MINUTES_PER_HOUR * SECONDS_PER_MINUTE) + _dt.minute() * SECONDS_PER_MINUTE + _dt.second();
    auto days = date.julian_day_number() - time::date::JULIAN_DAY_OF_19700101;
    return days * SECONDS_PER_DAY + seconds_in_day;
}

} // namespace hare
