#ifndef _HARE_BASE_FWD_INL_H_
#define _HARE_BASE_FWD_INL_H_

#include <hare/base/exception.h>
#include <hare/base/time/timestamp.h>

#include "base/thread/atomic_hook.h"

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

namespace hare {
namespace detail {

#define HARE_IMPL(Class, ...)                        \
  struct Class##Impl : public ::hare::detail::Impl { \
    __VA_ARGS__                                      \
    ~Class##Impl() override = default;               \
  };

#define HARE_IMPL_DPTR(Class)                                        \
  HARE_INLINE auto d_ptr(::hare::detail::Impl* impl)->Class##Impl* { \
    return ::hare::DownCast<Class##Impl*>(impl);                     \
  }

#define HARE_IMPL_DEFAULT(Class, ...) \
  HARE_IMPL(Class, __VA_ARGS__)       \
  HARE_IMPL_DPTR(Class)

#define IMPL d_ptr(impl_)

HARE_API void DefaultMsgHandle(std::uint8_t, const std::string& msg);
}  // namespace detail

HARE_INLINE
auto InnerLog() -> AtomicHook<LogHandler>& {
  static AtomicHook<LogHandler> log_handler{detail::DefaultMsgHandle};
  return log_handler;
}

#ifdef HARE_DEBUG
#define HARE_INTERNAL_TRACE(fromat, ...) \
  InnerLog()(TRACE_MSG, fmt::format(fromat, ##__VA_ARGS__))
#else
#define HARE_INTERNAL_TRACE(fromat, ...)
#endif
#define HARE_INTERNAL_ERROR(fromat, ...) \
  InnerLog()(ERROR_MSG, fmt::format(fromat, ##__VA_ARGS__))
#define HARE_INTERNAL_FATAL(fromat, ...) \
  throw Exception(fmt::format(fromat, ##__VA_ARGS__))

template <typename T>
HARE_INLINE auto CheckNotNull(const char* _names, T* _ptr) -> T* {
  if (_ptr == nullptr) {
    HARE_INTERNAL_FATAL(_names);
  }
  return _ptr;
}

#define CHECK_NULL(val) \
  ::hare::CheckNotNull("'" #val "' must be non NULL.", (val))

HARE_API void Abort(const char* _errmsg);

#define HARE_ASSERT(x)                                                   \
  do {                                                                   \
    if (HARE_PREDICT_TRUE((x))) {                                        \
      fmt::print(stderr, "Assertion failed: {} ({}:{})\n", #x, __FILE__, \
                 __LINE__);                                              \
      IgnoreUnused(std::fflush(stderr));                                 \
      ::hare::Abort(#x);                                                 \
    }                                                                    \
  } while (false)

}  // namespace hare

#endif  // _HARE_BASE_FWD_INL_H_