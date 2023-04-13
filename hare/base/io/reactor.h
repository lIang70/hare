#ifndef _HARE_NET_REACTOR_H_
#define _HARE_NET_REACTOR_H_

#include <hare/base/io/cycle.h>
#include <hare/net/util.h>

#include <map>
#include <string>

namespace hare {
namespace io {

    class reactor : public non_copyable {
        using event_map = std::map<util_socket_t, ptr<event>>;

        cycle* owner_cycle_ { nullptr };
        event_map events_ {};

    public:
        static auto create_by_type(cycle::REACTOR_TYPE _type, cycle* _cycle) -> reactor*;

        virtual ~reactor() = default;

        /**
         * @brief Polls the I/O events.
         *   Must be called in the cycle thread.
         */
        virtual auto poll(int32_t _timeout_microseconds, cycle::event_list& _active_events) -> timestamp = 0;

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

        /**
         *  @brief Detects whether the event is in the reactor.
         *   Must be called in the cycle thread.
         */
        virtual auto event_check(ptr<event> _event) const -> bool;

    protected:
        explicit reactor(cycle* cycle);

    };

} // namespace io
} // namespace hare

#endif // !_HARE_NET_REACTOR_H_