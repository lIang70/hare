#ifndef _HARE_NET_REACTOR_H_
#define _HARE_NET_REACTOR_H_

#include <hare/base/io/cycle.h>
#include <hare/base/io/event.h>
#include <hare/base/logging.h>
#include <hare/net/util.h>

#include <map>
#include <queue>
#include <string>
#include <utility>

#define POLL_TIME_MICROSECONDS 10000

#define CHECK_EVENT(event, val) ((event) & (val))
#define SET_EVENT(event, val) ((event) |= (val))
#define CLEAR_EVENT(event, val) ((event) &= ~(val))

namespace hare {
namespace io {

    using event_map = std::map<event::id, ptr<event>>;

    namespace detail {

        class event_elem {
            event::ptr event_ { nullptr };
            uint8_t revents_ { EVENT_DEFAULT };

        public:
            event_elem(event::ptr _event, uint8_t _revents) : event_(std::move(_event)) , revents_(_revents) {}

            friend class io::cycle;
            friend void print_active_events();
        };

        using event_list = std::list<event_elem>;

        class timer_elem {
            event::id id_ { 0 };
            timestamp stamp_ { 0 };

        public:
            timer_elem(event::id _id, timestamp _stamp) : id_(_id) , stamp_(_stamp) {}

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

        using priority_timer = std::priority_queue<timer_elem, std::vector<timer_elem>, timer_priority>;

        struct thread_storage {
            cycle* cycle { nullptr };
            event::id event_id { 0 };
            event_map events {};
            std::map<util_socket_t, event::id> inverse_map {};
            priority_timer ptimer {};
            event_list active_events {};
        };

        extern thread_local thread_storage tstorage;

        inline auto get_wait_time() -> int32_t
        {
            if (tstorage.ptimer.empty()) {
                return POLL_TIME_MICROSECONDS;
            }
            auto time = static_cast<int32_t>(tstorage.ptimer.top().stamp_.microseconds_since_epoch() - timestamp::now().microseconds_since_epoch());
            return time <= 0 ? static_cast<int32_t>(1) : std::min(time, POLL_TIME_MICROSECONDS);
        }

        inline void print_active_events()
        {
            for (const auto& event_elem : detail::tstorage.active_events) {
                LOG_TRACE() << "event[" << event_elem.event_->fd() << "] debug info: " << event_elem.event_->event_string() << ".";
            }
        }
    } // namespace detail

    class reactor : public non_copyable {
        cycle* owner_cycle_ { nullptr };

    public:
        static auto create_by_type(cycle::REACTOR_TYPE _type, cycle* _cycle) -> reactor*;

        virtual ~reactor() = default;

        /**
         * @brief Polls the I/O events.
         *   Must be called in the cycle thread.
         */
        virtual auto poll(int32_t _timeout_microseconds) -> timestamp = 0;

        /**
         *  @brief Changes the interested I/O events.
         *   Must be called in the cycle thread.
         */
        virtual void event_update(ptr<event> _event) = 0;

        /**
         *  @brief Remove the event, when it destructs.
         *   Must be called in the cycle thread.
         */
        virtual void event_remove(ptr<event> _event) = 0;

    protected:
        explicit reactor(cycle* cycle);
    };

} // namespace io
} // namespace hare

#endif // !_HARE_NET_REACTOR_H_