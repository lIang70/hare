#ifndef _HARE_BASE_IO_LOCAL_H_
#define _HARE_BASE_IO_LOCAL_H_

#include <hare/base/io/event.h>
#include <hare/hare-config.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

#include <sstream>
#include <thread>

namespace hare {
namespace io {

    namespace detail {
        HARE_INLINE
        void default_msg_handle(const std::string& msg)
        {
            fmt::println(stdout, msg);
            ignore_unused(::fflush(stdout));
        }
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
        current_thread::get_tds().tid, __FILE__, __LINE__, __func__))
#else
#define MSG_TRACE(fromat, ...)
#endif
#define MSG_ERROR(fromat, ...) \
    msg()(fmt::format("[ERROR] " fromat, ##__VA_ARGS__))
#define MSG_FATAL(fromat, ...) \
    throw exception(fmt::format(fromat, ##__VA_ARGS__))

    class cycle;

    namespace current_thread {

        using TID = struct thread_io_data {
            // io
            io::cycle* cycle { nullptr };
            io::event::id event_id { 0 };

            // thread
            std::uint64_t tid {};

            thread_io_data()
            {
                std::ostringstream oss {};
                oss << std::this_thread::get_id();
                tid = std::stoull(oss.str());
            }

            thread_io_data(const thread_io_data&) = delete;
            thread_io_data(thread_io_data&&) noexcept = delete;

            thread_io_data& operator=(const thread_io_data&) = delete;
            thread_io_data& operator=(thread_io_data&&) = delete;
        };

        HARE_INLINE
        auto get_tds() -> TID&
        {
            static thread_local struct thread_io_data t { };
            return t;
        }

    } // namespace current_thread

} // namespace io
} // namespace hare

#endif // !_HARE_BASE_IO_LOCAL_H_