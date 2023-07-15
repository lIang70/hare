#include "hare/base/io/local.h"
#include <hare/base/time/timezone.h>
#include <hare/log/details/msg.h>

namespace hare {
namespace log {
    namespace details {

        static auto log_time(const msg& _msg, std::int32_t& _microseconds) -> const std::string&
        {
            static thread_local std::int64_t t_last_time {};
            static thread_local std::string t_time {};
            auto microseconds_since_epoch = _msg.stamp_.microseconds_since_epoch();
            auto seconds = static_cast<time_t>(microseconds_since_epoch / HARE_MICROSECONDS_PER_SECOND);
            _microseconds = static_cast<std::int32_t>(microseconds_since_epoch - seconds * HARE_MICROSECONDS_PER_SECOND);
            
            assert(_msg.timezone_ != nullptr);

            if (seconds != t_last_time) {
                t_last_time = seconds;
                struct time::date_time dt {};
                if (bool(*_msg.timezone_)) {
                    dt = _msg.timezone_->to_local(seconds);
                } else {
                    dt = timezone::to_utc_time(seconds);
                }

                t_time = fmt::format("{:04}{:02}{:02} {:02}:{:02}:{:02}", dt.year_, dt.month_, dt.day_, dt.hour_, dt.minute_, dt.second_);
            }

            return t_time;
        }

        msg::msg(const std::string* _name, const timezone* _timezone, LEVEL _level, source_loc _loc)
            : name_(_name)
            , timezone_(_timezone)
            , level_(_level)
            , tid_(io::current_thread::get_tds().tid)
            , loc_(_loc)
        {
        }

        void format_msg(msg& _msg, msg_buffer_t& _fotmatted)
        {
            auto microseconds { 0 };
            const auto& stamp = log_time(_msg, microseconds);
            const auto* level = to_str(_msg.level_);
            _msg.raw_[_msg.raw_.size()] = '\0';
            
            // [LEVEL] (stamp) <tid> msg (loc)
            fmt::format_to(std::back_inserter(_fotmatted), 
                "[{:8}] ({:}.{:06d}) <{:#x}> {} [{}:{}||{}]" HARE_EOL, 
                level, stamp, microseconds, io::current_thread::get_tds().tid, 
                _msg.raw_.data(),
                _msg.loc_.filename_, _msg.loc_.line_, _msg.loc_.funcname_);
        }

    } // namespace details
} // namespace log
} // namespace hare