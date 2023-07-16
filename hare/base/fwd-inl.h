#ifndef _HARE_BASE_FWD_INL_H_
#define _HARE_BASE_FWD_INL_H_

#include <hare/base/time/timestamp.h>
#include <hare/base/exception.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

namespace hare {

namespace detail {
    extern void default_msg_handle(const std::string& msg);
} // namespace detail

HARE_INLINE
auto msg() -> std::function<void(std::string)>&
{
    static std::function<void(std::string)> s_io_msg {
        detail::default_msg_handle
    };
    return s_io_msg;
}

#ifdef HARE_DEBUG
#define MSG_TRACE(fromat, ...)                                \
    msg()(fmt::format("[TRACE] " fromat " [{:#x} {}:{}||{}]", \
        ##__VA_ARGS__,                                        \
        io::current_thread::get_tds().tid, __FILE__, __LINE__, __func__))
#else
#define MSG_TRACE(fromat, ...)
#endif
#define MSG_ERROR(fromat, ...) \
    msg()(fmt::format("[ERROR] " fromat, ##__VA_ARGS__))
#define MSG_FATAL(fromat, ...) \
    throw exception(fmt::format(fromat, ##__VA_ARGS__))

template <typename T>
HARE_INLINE
auto check_not_null(const char *_names, T* _ptr) -> T*
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