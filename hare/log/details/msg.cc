#include <hare/base/time/timezone.h>
#include <hare/log/details/async_msg.h>

#include "base/fwd-inl.h"
#include "base/thread/store.h"

namespace hare {
namespace log {

static auto LogTime(const Msg& _msg, std::int32_t& _microseconds)
    -> const std::string& {
  static thread_local std::int64_t t_last_time{};
  static thread_local std::string t_time{};

  auto microseconds_since_epoch = _msg.stamp_.microseconds_since_epoch();
  auto seconds = static_cast<time_t>(microseconds_since_epoch /
                                     HARE_MICROSECONDS_PER_SECOND);
  _microseconds = static_cast<std::int32_t>(
      microseconds_since_epoch - seconds * HARE_MICROSECONDS_PER_SECOND);

  HARE_ASSERT(_msg.timezone_ != nullptr);

  if (seconds != t_last_time) {
    t_last_time = seconds;
    struct DateTime dt {};
    if (bool(*_msg.timezone_)) {
      dt = _msg.timezone_->ToLocal(seconds);
    } else {
      dt = Timezone::ToUtcTime(seconds);
    }

    t_time = dt.ToFmt();
  }

  return t_time;
}

Msg::Msg(const std::string* _name, const hare::Timezone* _tz, Level _level,
         SourceLoc& _loc)
    : name_(_name),
      timezone_(_tz),
      level_(_level),
      tid_(::hare::ThreadStoreData().tid),
      loc_(_loc) {}

void FormatMsg(Msg& _msg, msg_buffer_t& _fotmatted) {
  auto microseconds{0};
  const auto& stamp = LogTime(_msg, microseconds);
  const auto* level = ToStr(static_cast<Level>(_msg.level_));
  _msg.raw_[_msg.raw_.size()] = '\0';

  // [LEVEL] (stamp) <tid> msg (loc)
  fmt::format_to(std::back_inserter(_fotmatted),
                 "({:}.{:06d}) [{:8}] <{:#x}> {} [{}:{}||{}]" HARE_EOL, stamp,
                 microseconds, level, ::hare::ThreadStoreData().tid,
                 _msg.raw_.data(), _msg.loc_.filename, _msg.loc_.line,
                 _msg.loc_.funcname);
}

}  // namespace log
}  // namespace hare
