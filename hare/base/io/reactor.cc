#include "base/io/reactor.h"

#include <hare/base/exception.h>
#include <hare/base/io/event.h>
#include <hare/hare-config.h>

#include "base/fwd-inl.h"

#if HARE__HAVE_EPOLL
#include "base/io/reactor/reactor_epoll.h"
#endif

#if HARE__HAVE_POLL
#include "base/io/reactor/reactor_poll.h"
#endif

namespace hare {
namespace io {

auto Reactor::CreateByType(Cycle::REACTOR_TYPE _type, Cycle* _cycle)
    -> Reactor* {
  switch (_type) {
    case Cycle::REACTOR_TYPE_EPOLL:
#if HARE__HAVE_POLL
      return new ReactorEpoll(_cycle);
#else
      HARE_INTERNAL_FATAL("epoll reactor was not supported.");
#endif
    case Cycle::REACTOR_TYPE_POLL:
#if HARE__HAVE_POLL
      return new ReactorPoll(_cycle);
#else
      HARE_INTERNAL_FATAL("poll reactor was not supported.");
#endif
    default:
      HARE_INTERNAL_FATAL("a suitable reactor type was not found.");
      return nullptr;
  }
}

Reactor::Reactor(Cycle* _cycle, Cycle::REACTOR_TYPE _type)
    : type_(_type), owner_cycle_(_cycle) {}

}  // namespace io
}  // namespace hare
