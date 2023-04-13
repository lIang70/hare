#include "hare/base/io/reactor.h"
#include <hare/base/exception.h>
#include <hare/base/io/event.h>
#include <hare/hare-config.h>

#ifdef HARE__HAVE_EPOLL
#include "hare/base/io/reactor/reactor_epoll.h"
#endif

#ifdef HARE__HAVE_POLL
#include "hare/base/io/reactor/reactor_poll.h"
#endif

namespace hare {
namespace io {

    auto reactor::create_by_type(cycle::REACTOR_TYPE _type, cycle* _cycle) -> reactor*
    {

        switch (_type) {
        case cycle::REACTOR_TYPE_EPOLL:
#ifdef HARE__HAVE_POLL
            return new reactor_epoll(_cycle);
#else
            throw exception("EPOLL reactor was not supported.");
#endif
        case cycle::REACTOR_TYPE_POLL:
#ifdef HARE__HAVE_POLL
            return new reactor_poll(_cycle);
#else
            throw exception("POLL reactor was not supported.");
#endif
        default:
            throw exception("A suitable reactor type was not found.");
        }
    }

    auto reactor::event_check(ptr<event> _event) const -> bool
    {
        if (_event) {
            return false;
        }
        auto iter = events_.find(_event->fd());
        return iter != events_.end() && iter->second == _event;
    }

    reactor::reactor(cycle* _cycle)
        : owner_cycle_(_cycle)
    {
    }

} // namespace io
} // namespace hare
