#include <hare/base/detail/datetime.h>
#include <hare/base/logging.h>
#include <hare/base/time_zone.h>

#include <algorithm>
#include <string>
#include <vector>

namespace hare {

namespace detail {

    static const int32_t SECONDS_PER_DAY = 24 * 60 * 60;

    inline void fillHMS(uint32_t seconds, struct time::DateTime* dt)
    {
        auto minutes = seconds / 60;
        dt->hour = minutes / 60;
        dt->minute = minutes - dt->hour * 60;
        dt->second = seconds - minutes * 60;
    }

    time::DateTime breakTime(int64_t t)
    {
        struct time::DateTime dt;
        auto seconds = static_cast<int32_t>(t % SECONDS_PER_DAY);
        auto days = static_cast<int32_t>(t / SECONDS_PER_DAY);
        // C++11 rounds towards zero.
        if (seconds < 0) {
            seconds += SECONDS_PER_DAY;
            --days;
        }
        detail::fillHMS(seconds, &dt);
        time::Date date(days + time::Date::JULIAN_DAY_OF_19700101);
        time::Date::YearMonthDay ymd = date.yearMonthDay();
        dt.year = ymd.year;
        dt.month = ymd.month;
        dt.day = ymd.day;

        return dt;
    }

} // namespace detail

struct TimeZone::Data {
    struct Transition {
        int64_t utc_time;
        int64_t local_time; // Shifted Epoch
        int32_t local_time_idx;

        Transition(int64_t t, int64_t l, int32_t local_idx)
            : utc_time(t)
            , local_time(l)
            , local_time_idx(local_idx)
        {
        }
    };

    struct CompareUtcTime {
        bool operator()(const Transition& lhs, const Transition& rhs) const
        {
            return lhs.utc_time < rhs.utc_time;
        }
    };

    struct CompareLocalTime {
        bool operator()(const Transition& lhs, const Transition& rhs) const
        {
            return lhs.local_time < rhs.local_time;
        }
    };

    struct LocalTime {
        int32_t utc_offset; // East of UTC
        bool is_dst;
        int32_t desig_idx;

        LocalTime(int32_t offset, bool dst, int32_t idx)
            : utc_offset(offset)
            , is_dst(dst)
            , desig_idx(idx)
        {
        }
    };

    std::vector<Transition> transitions;
    std::vector<LocalTime> local_times;
    std::string abbreviation;
    std::string tz_string;

    void addLocalTime(int32_t utc_offset, bool is_dst, int32_t desig_idx)
    {
        local_times.emplace_back(utc_offset, is_dst, desig_idx);
    }

    void addTransition(int64_t utc_time, int32_t local_time_idx)
    {
        auto& lt = local_times.at(local_time_idx);
        transitions.emplace_back(utc_time, utc_time + lt.utc_offset, local_time_idx);
    }

    const LocalTime* findLocalTime(int64_t utc_time) const;
    const LocalTime* findLocalTime(const struct time::DateTime& local, bool post_transition) const;
};

const TimeZone::Data::LocalTime* TimeZone::Data::findLocalTime(int64_t utc_time) const
{
    const LocalTime* local = nullptr;

    // row UTC time             isdst  offset  Local time (PRC)
    //  1  1989-09-16 17:00:00Z   0      8.0   1989-09-17 01:00:00
    //  2  1990-04-14 18:00:00Z   1      9.0   1990-04-15 03:00:00
    //  3  1990-09-15 17:00:00Z   0      8.0   1990-09-16 01:00:00
    //  4  1991-04-13 18:00:00Z   1      9.0   1991-04-14 03:00:00
    //  5  1991-09-14 17:00:00Z   0      8.0   1991-09-15 01:00:00

    // input '1990-06-01 00:00:00Z', std::upper_bound returns row 3,
    // so the input is in range of row 2, offset is 9 hours,
    // local time is 1990-06-01 09:00:00
    if (transitions.empty() || utc_time < transitions.front().utc_time) {
        // FIXME: should be first non dst time zone
        local = &local_times.front();
    } else {
        Transition sentry(utc_time, 0, 0);
        auto trans_iter = std::upper_bound(transitions.begin(), transitions.end(), sentry, CompareUtcTime());
        HARE_ASSERT(trans_iter != transitions.begin(), "Fail to upper_bound.");
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

const TimeZone::Data::LocalTime* TimeZone::Data::findLocalTime(const struct time::DateTime& lt, bool post_transition) const
{
    const auto local_time = fromUtcTime(lt);

    if (transitions.empty() || local_time < transitions.front().local_time) {
        // FIXME: should be first non dst time zone
        return &local_times.front();
    }

    Transition sentry(0, local_time, 0);
    auto trans_iter = std::upper_bound(transitions.begin(), transitions.end(), sentry, CompareLocalTime());
    HARE_ASSERT(trans_iter != transitions.begin(), "Fail to upper_bound.");

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
        if (post_transition) {
            return &local_times[trans_iter->local_time_idx];
        } else {
            return &local_times[prior_trans.local_time_idx];
        }
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
        if (post_transition) {
            return &local_times[trans_iter->local_time_idx];
        } else {
            return &local_times[prior_trans.local_time_idx];
        }
    }

    // otherwise, it's unique
    return &local_times[trans_iter->local_time_idx];
}

// static
TimeZone TimeZone::UTC()
{
    return TimeZone(0, "UTC");
}

TimeZone::TimeZone(int32_t east_of_utc, const char* name)
    : d_(new Data)
{
    d_->addLocalTime(east_of_utc, false, 0);
    d_->abbreviation = name;
}

TimeZone::TimeZone(const TimeZone& tz)
{
    if (tz) {
        d_ = new Data;
        *d_ = *tz.d_;
    }
}

TimeZone::~TimeZone()
{
    delete d_;
}

TimeZone& TimeZone::operator=(const TimeZone& tz)
{
    if (tz) {
        if (!d_)
            d_ = new Data;
        *d_ = *tz.d_;
    }
    return (*this);
}

struct time::DateTime TimeZone::toLocalTime(int64_t seconds, int32_t* utc_offset) const
{
    struct time::DateTime localTime;
    HARE_ASSERT(d_, "");

    auto local = d_->findLocalTime(seconds);

    if (local) {
        localTime = detail::breakTime(seconds + local->utc_offset);
        if (utc_offset) {
            *utc_offset = local->utc_offset;
        }
    }

    return localTime;
}

int64_t TimeZone::fromLocalTime(const struct time::DateTime& localtime, bool post_transition) const
{
    HARE_ASSERT(d_, "Fail to get data of TimeZone.");
    auto local = d_->findLocalTime(localtime, post_transition);
    const auto local_seconds = fromUtcTime(localtime);
    if (local) {
        return local_seconds - local->utc_offset;
    }
    // fallback as if it's UTC time.
    return local_seconds;
}

time::DateTime TimeZone::toUtcTime(int64_t secondsSinceEpoch)
{
    return detail::breakTime(secondsSinceEpoch);
}

int64_t TimeZone::fromUtcTime(const DateTime& dt)
{
    time::Date date(dt.year, dt.month, dt.day);
    auto seconds_in_day = dt.hour * 3600 + dt.minute * 60 + dt.second;
    int64_t days = date.julianDayNumber() - time::Date::JULIAN_DAY_OF_19700101;
    return days * detail::SECONDS_PER_DAY + seconds_in_day;
}

} // namespace hare
