/**
 * @file hare/log/async_logger.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with async_logger.h
 * @version 0.1-beta
 * @date 2023-07-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_ASYNC_MSG_H_
#define _HARE_LOG_ASYNC_MSG_H_

#include <hare/log/details/msg.h>

namespace hare {
namespace log {
namespace detail {

HARE_CLASS_API
struct HARE_API AsyncMsg : public Msg {
  enum Type { LOG, FLUSH, TERMINATE };
  Type type_{LOG};

  AsyncMsg() = default;

  explicit AsyncMsg(Type _type) : type_(_type) {}

  AsyncMsg(Msg& _msg, Type _type) : Msg(std::move(_msg)), type_(_type) {}

  HARE_INLINE
  AsyncMsg(AsyncMsg&& _other) noexcept { Move(_other); }

  HARE_INLINE
  auto operator=(AsyncMsg&& _other) noexcept -> AsyncMsg& {
    Move(_other);
    return (*this);
  }

  HARE_INLINE
  void Move(AsyncMsg& _other) noexcept {
    name_ = _other.name_;
    timezone_ = _other.timezone_;
    level_ = _other.level_;
    tid_ = _other.tid_;
    id_ = _other.id_;
    stamp_ = _other.stamp_;
    raw_ = std::move(_other.raw_);
    loc_ = _other.loc_;
    type_ = _other.type_;
  }
};

}  // namespace detail
}  // namespace log
}  // namespace hare

#endif  // _HARE_LOG_ASYNC_MSG_H_