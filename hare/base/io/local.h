#ifndef _HARE_BASE_LOCAL_H_
#define _HARE_BASE_LOCAL_H_

#include <hare/base/io/event.h>
#include <hare/hare-config.h>

#include <list>
#include <map>
#include <queue>

namespace hare {
namespace io {
    class cycle;

    class event_elem {
        event::ptr event_ { nullptr };
        uint8_t revents_ { io::EVENT_DEFAULT };

    public:
        event_elem(event::ptr _event, uint8_t _revents)
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
        friend auto get_wait_time() -> int32_t;
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

} // namespace io

namespace current_thread {

    using TID = struct thread_io_data {        
        io::cycle* cycle { nullptr };
        io::event::id event_id { 0 };
        io::event_map events {};
        std::map<util_socket_t, io::event::id> inverse_map {};
        io::priority_timer ptimer {};
        io::events_list active_events {};
    };

    inline auto get_tds() -> TID&
    {
        static thread_local struct thread_io_data t;
        return t;
    }

} // namespace current_thread
} // namespace hare

#endif // !_HARE_BASE_LOCAL_H_