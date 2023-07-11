#ifndef _HARE_NET_REACTOR_H_
#define _HARE_NET_REACTOR_H_

#include <hare/base/io/cycle.h>
#include <hare/base/io/event.h>

#include <string>

#define POLL_TIME_MICROSECONDS 10000

#define CHECK_EVENT(event, val) ((event) & (val))
#define SET_EVENT(event, val) ((event) |= (val))
#define CLEAR_EVENT(event, val) ((event) &= ~(val))

namespace hare {
namespace io {

    class reactor : public util::non_copyable {
        cycle::REACTOR_TYPE type_ {};
        cycle* owner_cycle_ { nullptr };

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
        virtual void event_update(ptr<event> _event) = 0;

        /**
         *  @brief Remove the event, when it destructs.
         *   Must be called in the cycle thread.
         */
        virtual void event_remove(ptr<event> _event) = 0;

    protected:
        explicit reactor(cycle* cycle, cycle::REACTOR_TYPE _type);
    };

} // namespace io
} // namespace hare

#endif // !_HARE_NET_REACTOR_H_