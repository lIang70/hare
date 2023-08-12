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

namespace hare {
namespace io {

    auto Reactor::CreateByType(Cycle::REACTOR_TYPE _type, Cycle* _cycle) -> Reactor*
    {
        switch (_type) {
        case Cycle::REACTOR_TYPE_EPOLL:
#if HARE__HAVE_POLL
            return new ReactorEpoll(_cycle);
#else
            throw Exception("EPOLL reactor was not supported.");
#endif
        case Cycle::REACTOR_TYPE_POLL:
#if HARE__HAVE_POLL
            return new ReactorPoll(_cycle);
#else
            throw Exception("POLL reactor was not supported.");
#endif
        default:
            throw Exception("A suitable reactor type was not found.");
        }
    }

    Reactor::Reactor(Cycle* _cycle, Cycle::REACTOR_TYPE _type)
        : type_(_type)
        , owner_cycle_(_cycle)
    {
    }

} // namespace io
} // namespace hare
