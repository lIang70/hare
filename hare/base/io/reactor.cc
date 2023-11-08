#include "base/io/reactor.h"

#include <hare/base/exception.h>
#include <hare/base/io/event.h>
#include <hare/hare-config.h>

#include "base/fwd_inl.h"
#include "base/platforms/linux/reactor/reactor_epoll.h"
#include "base/platforms/linux/reactor/reactor_poll.h"

namespace hare {

auto Reactor::CreateByType(Cycle::REACTOR_TYPE _type) -> Reactor* {
  switch (_type) {
    case Cycle::REACTOR_TYPE_EPOLL:
#if HARE__HAVE_POLL
      return new ReactorEpoll();
#else
      HARE_INTERNAL_FATAL("epoll reactor was not supported.");
#endif
    case Cycle::REACTOR_TYPE_POLL:
#if HARE__HAVE_POLL
      return new ReactorPoll();
#else
      HARE_INTERNAL_FATAL("poll reactor was not supported.");
#endif
    default:
      HARE_INTERNAL_FATAL("a suitable reactor type was not found.");
      return nullptr;
  }
}

void Reactor::AddEvent(Event* event, bool need_free) {
  HARE_ASSERT(!this->Check(event));
  if (Check(event)) {
    events_.insert(std::make_pair(
        event->id(), io_inner::EventElem{event->id(), event, 0, need_free}));
  }
  if (CHECK_EVENT(event->events(), EVENT_TIMEOUT) != 0) {
    ptimer_.emplace(event->id(),
                    Timestamp(Timestamp::Now().microseconds_since_epoch() +
                              event->timeval()));
  }
}

Reactor::Reactor(Cycle::REACTOR_TYPE _type) : type_(_type) {}

}  // namespace hare
