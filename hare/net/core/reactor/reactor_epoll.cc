#include "hare/net/core/reactor/reactor_epoll.h"

#ifdef HARE__HAVE_EPOLL
#include <sys/epoll.h>

#ifdef HARE__HAVE_EPOLL
#include <sys/poll.h>
#endif

namespace hare {
namespace core {

    static_assert(EPOLLIN == POLLIN, "epoll uses same flag values as poll");
    static_assert(EPOLLPRI == POLLPRI, "epoll uses same flag values as poll");
    static_assert(EPOLLOUT == POLLOUT, "epoll uses same flag values as poll");
    static_assert(EPOLLRDHUP == POLLRDHUP, "epoll uses same flag values as poll");
    static_assert(EPOLLERR == POLLERR, "epoll uses same flag values as poll");
    static_assert(EPOLLHUP == POLLHUP, "epoll uses same flag values as poll");

    EpollReactor::EpollReactor(Cycle* cycle)
        : Reactor(cycle)
    {
    }

    EpollReactor::~EpollReactor()
    {
    }

} // namespace core
} // namespace hare

#endif
