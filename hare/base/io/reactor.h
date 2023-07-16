#ifndef _HARE_BASE_IO_REACTOR_H_
#define _HARE_BASE_IO_REACTOR_H_

#include <hare/base/io/cycle.h>
#include <hare/base/io/event.h>

#include <list>
#include <map>
#include <queue>

#define POLL_TIME_MICROSECONDS 1000000

#define CHECK_EVENT(event, val) ((event) & (val))
#define SET_EVENT(event, val) ((event) |= (val))
#define CLEAR_EVENT(event, val) ((event) &= ~(val))

namespace hare {
namespace io {

    namespace detail {
        struct event_elem {
            event::ptr event_ { nullptr };
            std::uint8_t revents_ { io::EVENT_DEFAULT };

            event_elem(event::ptr _event, std::uint8_t _revents)
                : event_(std::move(_event))
                , revents_(_revents)
            {
            }
        };

        struct timer_elem {
            event::id id_ { 0 };
            timestamp stamp_ { 0 };

            timer_elem(event::id _id, timestamp _stamp)
                : id_(_id)
                , stamp_(_stamp)
            {
            }
        };

        struct timer_priority {
            auto operator()(const timer_elem& _elem_x, const timer_elem& _elem_y) -> bool
            {
                return _elem_x.stamp_ < _elem_y.stamp_;
            }
        };

    } // namespace detail

    using events_list = std::list<detail::event_elem>;
    using event_map = std::map<event::id, ptr<event>>;
    using priority_timer = std::priority_queue<
        detail::timer_elem,
        std::vector<detail::timer_elem>,
        detail::timer_priority>;

    class reactor : public util::non_copyable {
        cycle::REACTOR_TYPE type_ {};
        cycle* owner_cycle_ { nullptr };

    protected:
        event_map events_ {};
        std::map<util_socket_t, io::event::id> inverse_map_ {};
        priority_timer ptimer_ {};
        events_list active_events_ {};

    public:
        static auto create_by_type(cycle::REACTOR_TYPE _type, cycle* _cycle) -> reactor*;

        virtual ~reactor() = default;

        HARE_INLINE
        auto type() -> cycle::REACTOR_TYPE { return type_; }

        /**
         * @brief Polls the I/O events.
         *   Must be called in the cycle thread.
         */
        virtual auto poll(std::int32_t _timeout_microseconds) -> timestamp = 0;

        /**
         *  @brief Changes the interested I/O events.
         *   Must be called in the cycle thread.
         */
        virtual auto event_update(const ptr<event>& _event) -> bool = 0;

        /**
         *  @brief Remove the event, when it destructs.
         *   Must be called in the cycle thread.
         */
        virtual auto event_remove(const ptr<event>& _event) -> bool = 0;

    protected:
        explicit reactor(cycle* cycle, cycle::REACTOR_TYPE _type);

        friend class io::cycle;
    };

} // namespace io
} // namespace hare

#endif // _HARE_BASE_IO_REACTOR_H_