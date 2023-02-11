#include "hare/net/core/reactor.h"
#include <hare/base/exception.h>
#include <hare/net/event.h>

#ifdef HARE__HAVE_EPOLL
#include "hare/net/core/reactor/reactor_epoll.h"
#endif

namespace hare {
namespace core {

    Reactor* Reactor::createByType(const std::string& type, Cycle* cycle)
    {
        if (type == "EPOLL")
#ifdef HARE__HAVE_EPOLL
            return new EpollReactor(cycle);
#else
            throw Exception("EPOLL reactor was not supported.");
#endif

        throw Exception("A suitable reactor type was not found.");
        return nullptr;
    }

    bool Reactor::checkEvent(Event* event) const
    {
        assertInLoopThread();
        auto iter = events_.find(event->fd());
        return iter != events_.end() && iter->second == event;
    }

} // namespace core
} // namespace hare
