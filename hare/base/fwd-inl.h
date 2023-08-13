#ifndef _HARE_BASE_FWD_INL_H_
#define _HARE_BASE_FWD_INL_H_

#include <hare/base/exception.h>
#include <hare/base/time/timestamp.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

namespace hare {
namespace detail {

#define HARE_IMPL_DEFAULT(Class, ...)                \
    struct Class##Impl : public hare::detail::Impl { \
        __VA_ARGS__                                  \
        ~Class##Impl() override = default;           \
    };                                               \
    HARE_INLINE auto d_ptr(hare::detail::Impl* impl) \
        ->Class##Impl* { return down_cast<Class##Impl*>(impl); }

#define HARE_IMPL(Class, ...)                        \
    struct Class##Impl : public hare::detail::Impl { \
        __VA_ARGS__                                  \
    };                                               \
    HARE_INLINE auto d_ptr(hare::detail::Impl* impl) \
        ->Class##Impl* { return down_cast<Class##Impl*>(impl); }

    HARE_API void DefaultMsgHandle(std::uint8_t, const std::string& msg);
} // namespace detail

HARE_INLINE
auto InnerLog() -> LogHandler&
{
    static LogHandler log_handler {
        detail::DefaultMsgHandle
    };
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
HARE_INLINE
auto CheckNotNull(const char* _names, T* _ptr) -> T*
{
    if (_ptr == nullptr) {
        HARE_INTERNAL_FATAL(_names);
    }
    return _ptr;
}

#define CHECK_NULL(val) \
    ::hare::CheckNotNull("'" #val "' must be non NULL.", (val))

} // namespace hare

#endif // _HARE_BASE_FWD_INL_H_