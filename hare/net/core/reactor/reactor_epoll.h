#ifndef _HARE_NET_REACTOR_EPOLL_H_
#define _HARE_NET_REACTOR_EPOLL_H_

#include "hare/net/core/reactor.h"

namespace hare {
namespace core {

    class EpollReactor : public Reactor {
        using EPEventList = std::vector<struct epoll_event>;

        socket_t epoll_fd_;
        EPEventList epoll_events_;

    public:
        EpollReactor(Cycle* cycle);
        ~EpollReactor() override;

        Timestamp poll(int timeout_microseconds, Cycle::EventList& active_events) override;
        void updateEvent(Event* event) override;
        void removeEvent(Event* event) override;

    private:
        void fillActiveEvents(int num_of_events, Cycle::EventList& active_events) const;
        void update(int operation, Event* channel);
    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_REACTOR_EPOLL_H_