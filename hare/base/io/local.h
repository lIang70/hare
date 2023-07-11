#ifndef _HARE_BASE_IO_LOCAL_H_
#define _HARE_BASE_IO_LOCAL_H_

#include <hare/base/io/event.h>
#include <hare/hare-config.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

#include <list>
#include <map>
#include <queue>
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
        current_thread::get_tds().tid, __FILE__, __LINE__, __func__));
#else
#define MSG_TRACE(fromat, ...)
#endif

    class cycle;

    class event_elem {
        event::ptr event_ { nullptr };
        std::uint8_t revents_ { io::EVENT_DEFAULT };

    public:
        event_elem(event::ptr _event, std::uint8_t _revents)
            : event_(std::move(_event))
            , revents_(_revents)
        {
        }

        friend class cycle;
        friend void print_active_events();
    };

    class timer_elem {
        event::id id_ { 0 };
        timestamp stamp_ { 0 };

    public:
        timer_elem(event::id _id, timestamp _stamp)
            : id_(_id)
            , stamp_(_stamp)
        {
        }

        friend class io::cycle;
        friend struct timer_priority;
        friend auto get_wait_time() -> std::int32_t;
    };

    struct timer_priority {
        auto operator()(const timer_elem& _elem_x, const timer_elem& _elem_y) -> bool
        {
            return _elem_x.stamp_ < _elem_y.stamp_;
        }
    };

    using events_list = std::list<event_elem>;
    using priority_timer = std::priority_queue<timer_elem, std::vector<timer_elem>, timer_priority>;
    using event_map = std::map<event::id, ptr<event>>;

    namespace current_thread {

        using TID = struct thread_io_data {
            // io
            io::cycle* cycle { nullptr };
            io::event::id event_id { 0 };
            io::event_map events {};
            std::map<util_socket_t, io::event::id> inverse_map {};
            io::priority_timer ptimer {};
            io::events_list active_events {};

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
            static thread_local struct thread_io_data t {
            };
            return t;
        }

    } // namespace current_thread

} // namespace io
} // namespace hare

#endif // !_HARE_BASE_IO_LOCAL_H_