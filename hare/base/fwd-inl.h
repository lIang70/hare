#ifndef _HARE_BASE_FWD_INL_H_
#define _HARE_BASE_FWD_INL_H_

#include <hare/base/exception.h>
#include <hare/base/time/timestamp.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

namespace hare {

namespace detail {
    extern void default_msg_handle(std::uint8_t, const std::string& msg);
} // namespace detail

HARE_INLINE
auto msg() -> std::function<void(std::uint8_t, std::string)>&
{
    static std::function<void(std::uint8_t, std::string)> s_io_msg {
        detail::default_msg_handle
    };
    return s_io_msg;
}

#ifdef HARE_DEBUG
#define MSG_TRACE(fromat, ...) \
    msg()(trace_msg, fmt::format(fromat, ##__VA_ARGS__))
#else
#define MSG_TRACE(fromat, ...)
#endif
#define MSG_ERROR(fromat, ...) \
    msg()(error_msg, fmt::format(fromat, ##__VA_ARGS__))
#define MSG_FATAL(fromat, ...) \
    throw exception(fmt::format(fromat, ##__VA_ARGS__))

template <typename T>
HARE_INLINE
auto check_not_null(const char* _names, T* _ptr) -> T*
{
    if (_ptr == nullptr) {
        MSG_FATAL(_names);
    }
    return _ptr;
}

#define CHECK_NULL(val) \
    ::hare::check_not_null("'" #val "' must be non NULL.", (val))

} // namespace hare

#endif // _HARE_BASE_FWD_INL_H_