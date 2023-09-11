/**
 * @file hare/log/details/msg.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with msg.h
 * @version 0.1-beta
 * @date 2023-07-12
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_DETAILS_MSG_H_
#define _HARE_LOG_DETAILS_MSG_H_

#include <hare/base/io/file.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/time/timezone.h>
#include <hare/base/util/non_copyable.h>
#include <hare/log/details/dummy.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

#include <unordered_map>

namespace hare {
namespace log {

/**
 * @brief The enumeration of log level.
 *
 **/
using Level = enum : std::int8_t {
  LEVEL_TRACE,
  LEVEL_DEBUG,
  LEVEL_INFO,
  LEVEL_WARNING,
  LEVEL_ERROR,
  LEVEL_FATAL,
  LEVEL_NBRS
};

#ifndef HARELOG_USING_ATOMIC_LEVELS
using level_t = detail::DummyAtomic<std::int8_t>;
#else
using level_t = std::atomic<std::int8_t>;
#endif

#if !defined(HARELOG_LEVEL_NAMES)
#define HARELOG_LEVEL_NAMES \
  { "trace", "debug", "info", "warning", "error", "fatal", "nbrs" }
#endif

static constexpr const char* level_name[] HARELOG_LEVEL_NAMES;

HARE_INLINE
constexpr const char* ToStr(Level _level) { return level_name[_level]; }

HARE_INLINE
Level FromStr(const std::string& name) {
  static std::unordered_map<std::string, Level> name_to_level = {
      {level_name[0], LEVEL_TRACE},    // trace
      {level_name[1], LEVEL_DEBUG},    // debug
      {level_name[2], LEVEL_INFO},     // info
      {level_name[3], LEVEL_WARNING},  // warning
      {level_name[4], LEVEL_ERROR},    // error
      {level_name[5], LEVEL_FATAL},    // critical
      {level_name[6], LEVEL_NBRS}      // nbrs
  };

  auto iter = name_to_level.find(name);
  return iter != name_to_level.end() ? iter->second : LEVEL_NBRS;
}

HARE_CLASS_API
struct HARE_API SourceLoc {
  const char* filename{nullptr};
  const char* funcname{nullptr};
  std::int32_t line{0};

  constexpr SourceLoc() = default;
  HARE_INLINE SourceLoc(const char* _filename, std::int32_t _line,
                        const char* _funcname)
      : funcname{_funcname}, line{_line} {
    const auto* slash = std::strrchr(_filename, HARE_SLASH);
    filename = slash != nullptr ? slash + 1 : _filename;
  }

  HARE_INLINE
  auto Empty() const noexcept -> bool { return line == 0; }
};

namespace detail {

using msg_buffer_t = fmt::basic_memory_buffer<char, 256>;

HARE_CLASS_API
struct HARE_API Msg : public util::NonCopyable {
  const std::string* name_{};
  const Timezone* timezone_{};
  std::int8_t level_{LEVEL_NBRS};
  std::uint64_t tid_{0};
  std::uint64_t id_{0};
  Timestamp stamp_{Timestamp::Now()};
  msg_buffer_t raw_{};
  SourceLoc loc_{};

  Msg() = default;
  virtual ~Msg() = default;

  Msg(const std::string* _name, const hare::Timezone* _tz, Level _level,
      SourceLoc& _loc);

  HARE_INLINE
  Msg(Msg&& _other) noexcept { Move(_other); }

  HARE_INLINE
  auto operator=(Msg&& _other) noexcept -> Msg& {
    Move(_other);
    return (*this);
  }

  HARE_INLINE
  void Move(Msg& _other) noexcept {
    name_ = _other.name_;
    timezone_ = _other.timezone_;
    level_ = _other.level_;
    tid_ = _other.tid_;
    id_ = _other.id_;
    stamp_ = _other.stamp_;
    raw_ = std::move(_other.raw_);
    loc_ = _other.loc_;
  }
};

HARE_API void FormatMsg(Msg& _msg, msg_buffer_t& _fotmatted);

}  // namespace detail
}  // namespace log
}  // namespace hare

#endif  // _HARE_LOG_DETAILS_MSG_H_
