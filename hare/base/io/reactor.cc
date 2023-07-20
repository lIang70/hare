#include "hare/base/io/reactor.h"
#include <hare/base/exception.h>
#include <hare/base/io/event.h>
#include <hare/hare-config.h>

#if HARE__HAVE_EPOLL
#include "hare/base/io/reactor/reactor_epoll.h"
#endif

#if HARE__HAVE_POLL
#include "hare/base/io/reactor/reactor_poll.h"
#endif

#if HARE__HAVE_SELECT || defined(H_OS_WIN)
#include "hare/base/io/reactor/reactor_select.h"
#endif

namespace hare {
namespace io {

    auto reactor::create_by_type(cycle::REACTOR_TYPE _type, cycle* _cycle) -> reactor*
    {
        switch (_type) {
        case cycle::REACTOR_TYPE_EPOLL:
#if HARE__HAVE_POLL
            return new reactor_epoll(_cycle);
#else
            throw exception("EPOLL reactor was not supported.");
#endif
        case cycle::REACTOR_TYPE_POLL:
#if HARE__HAVE_POLL
            return new reactor_poll(_cycle);
#else
            throw exception("POLL reactor was not supported.");
#endif
        case cycle::REACTOR_TYPE_SELECT:
#if HARE__HAVE_SELECT || defined(H_OS_WIN)
            return new reactor_select(_cycle);
#else
            throw exception("SELECT reactor was not supported.");
#endif
        default:
            throw exception("A suitable reactor type was not found.");
        }
    }

    reactor::reactor(cycle* _cycle, cycle::REACTOR_TYPE _type)
        : type_(_type)
        , owner_cycle_(_cycle)
    {
    }

} // namespace io
} // namespace hare
