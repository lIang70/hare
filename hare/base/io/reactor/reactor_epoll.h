#ifndef _HARE_NET_REACTOR_EPOLL_H_
#define _HARE_NET_REACTOR_EPOLL_H_

#include "hare/base/io/reactor.h"
#include <hare/hare-config.h>

#include <vector>

#if HARE__HAVE_EPOLL
#include <sys/epoll.h>

namespace hare {
namespace io {

    class reactor_epoll : public reactor {
        using ep_event_list = std::vector<struct epoll_event>;

        util_socket_t epoll_fd_ { -1 };
        ep_event_list epoll_events_ {};

    public:
        explicit reactor_epoll(cycle* cycle, cycle::REACTOR_TYPE _type);
        ~reactor_epoll() override;

        auto poll(std::int32_t _timeout_microseconds) -> timestamp override;
        void event_update(ptr<event> _event) override;
        void event_remove(ptr<event> _event) override;

    private:
        void fill_active_events(std::int32_t _num_of_events);
        void update(std::int32_t _operation, const ptr<event>& _event) const;
    };

} // namespace io
} // namespace hare

#endif // HARE__HAVE_EPOLL

#endif // !_HARE_NET_REACTOR_EPOLL_H_