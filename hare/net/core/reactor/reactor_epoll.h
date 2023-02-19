#ifndef _HARE_NET_REACTOR_EPOLL_H_
#define _HARE_NET_REACTOR_EPOLL_H_

#include "hare/net/core/reactor.h"

#ifdef HARE__HAVE_EPOLL
#include <sys/epoll.h>

namespace hare {
namespace core {

    class EpollReactor : public Reactor {
        using EPEventList = std::vector<struct epoll_event>;

        util_socket_t epoll_fd_ { -1 };
        EPEventList epoll_events_ {};

    public:
        EpollReactor(Cycle* cycle);
        ~EpollReactor() override;

        Timestamp poll(int32_t timeout_microseconds, Cycle::EventList& active_events) override;
        void updateEvent(Event* event) override;
        void removeEvent(Event* event) override;

    private:
        void fillActiveEvents(int32_t num_of_events, Cycle::EventList& active_events);
        void update(int32_t operation, Event* channel);
    };

} // namespace core
} // namespace hare

#endif // HARE__HAVE_EPOLL

#endif // !_HARE_NET_REACTOR_EPOLL_H_