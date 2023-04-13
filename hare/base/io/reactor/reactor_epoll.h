#ifndef _HARE_NET_REACTOR_EPOLL_H_
#define _HARE_NET_REACTOR_EPOLL_H_

#include "hare/base/io/reactor.h"
#include <hare/hare-config.h>

#ifdef HARE__HAVE_EPOLL
#include <sys/epoll.h>

namespace hare {
namespace io {

    class reactor_epoll : public reactor {
        using EPEventList = std::vector<struct epoll_event>;

        util_socket_t epoll_fd_ { -1 };
        EPEventList epoll_events_ {};

    public:
        explicit reactor_epoll(cycle* cycle);
        ~reactor_epoll() override;

        auto poll(int32_t _timeout_microseconds, cycle::event_list& _active_events) -> timestamp override;
        void event_update(ptr<event> _event) override;
        void event_remove(ptr<event> _event) override;

    private:
        void fillActiveEvents(int32_t _num_of_events, cycle::event_list& _active_events);
        void update(int32_t _operation, ptr<event> _event) const;
    };

} // namespace io
} // namespace hare

#endif // HARE__HAVE_EPOLL

#endif // !_HARE_NET_REACTOR_EPOLL_H_