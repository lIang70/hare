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
        struct EventElem {
            Ptr<Event> event { nullptr };
            std::uint8_t revents { io::EVENT_DEFAULT };

            EventElem(Ptr<Event> _event, std::uint8_t _revents)
                : event(std::move(_event))
                , revents(_revents)
            { }
        };

        struct TimerElem {
            Event::Id id { 0 };
            Timestamp stamp { 0 };

            TimerElem(Event::Id _id, Timestamp _stamp)
                : id(_id)
                , stamp(_stamp)
            {
            }
        };

        struct TimerPriority {
            auto operator()(const TimerElem& _elem_x, const TimerElem& _elem_y) -> bool
            {
                return _elem_x.stamp < _elem_y.stamp;
            }
        };

    } // namespace detail

    using EventsList = std::list<detail::EventElem>;
    using EventMap = std::map<Event::Id, Ptr<Event>>;
    using PriorityTimer = std::priority_queue<
        detail::TimerElem,
        std::vector<detail::TimerElem>,
        detail::TimerPriority>;

    class Reactor : public util::NonCopyable {
        Cycle::REACTOR_TYPE type_ {};
        Cycle* owner_cycle_ { nullptr };

    protected:
        EventMap events_ {};
        std::map<util_socket_t, io::Event::Id> inverse_map_ {};
        PriorityTimer ptimer_ {};
        EventsList active_events_ {};

    public:
        static auto CreateByType(Cycle::REACTOR_TYPE _type, Cycle* _cycle) -> Reactor*;

        virtual ~Reactor() = default;

        HARE_INLINE
        auto type() -> Cycle::REACTOR_TYPE { return type_; }

        /**
         * @brief Polls the I/O events.
         *   Must be called in the cycle thread.
         */
        virtual auto Poll(std::int32_t _timeout_microseconds) -> Timestamp = 0;

        /**
         *  @brief Changes the interested I/O events.
         *   Must be called in the cycle thread.
         */
        virtual auto EventUpdate(const Ptr<Event>& _event) -> bool = 0;

        /**
         *  @brief Remove the event, when it destructs.
         *   Must be called in the cycle thread.
         */
        virtual auto EventRemove(const Ptr<Event>& _event) -> bool = 0;

    protected:
        explicit Reactor(Cycle* cycle, Cycle::REACTOR_TYPE _type);

        friend class io::Cycle;
    };

} // namespace io
} // namespace hare

#endif // _HARE_BASE_IO_REACTOR_H_