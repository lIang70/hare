#include "reactor_epoll.h"

#include <hare/base/exception.h>
#include <hare/base/io/operation.h>

#include "base/fwd_inl.h"

#if HARE__HAVE_EPOLL && HARE__HAVE_UNISTD_H

#include <unistd.h>

#include <sstream>

namespace hare {

namespace io_inner {
const std::int32_t kInitEventsCnt = 16;

static auto OperationToString(std::int32_t _op) -> std::string {
  switch (_op) {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      HARE_ASSERT(false);
      return "Unknown Operation";
  }
}

static auto DecodeEpoll(std::uint8_t _events) -> decltype(epoll_event::events) {
  decltype(epoll_event::events) events = 0;
  if (CHECK_EVENT(_events, EVENT_READ) != 0) {
    SET_EVENT(events, EPOLLIN | EPOLLPRI);
  }
  if (CHECK_EVENT(_events, EVENT_WRITE) != 0) {
    SET_EVENT(events, EPOLLOUT);
  }
  if (CHECK_EVENT(_events, EVENT_ET) != 0) {
    SET_EVENT(events, EPOLLET);
  }
  return events;
}

static auto EncodeEpoll(decltype(epoll_event::events) _events) -> std::uint8_t {
  std::uint8_t events{EVENT_DEFAULT};

  if ((CHECK_EVENT(_events, EPOLLERR) != 0) ||
      (CHECK_EVENT(_events, EPOLLHUP) != 0 &&
       CHECK_EVENT(_events, EPOLLRDHUP) == 0)) {
    SET_EVENT(events, EVENT_READ | EVENT_WRITE);
  } else {
    if (CHECK_EVENT(_events, EPOLLIN) != 0) {
      SET_EVENT(events, EVENT_READ);
    }
    if (CHECK_EVENT(_events, EPOLLOUT) != 0) {
      SET_EVENT(events, EVENT_WRITE);
    }
    if (CHECK_EVENT(_events, EPOLLRDHUP) != 0) {
      SET_EVENT(events, EVENT_CLOSED);
    }
  }

  return events;
}

static auto EpollToString(decltype(epoll_event::events) _epoll_event)
    -> std::string {
  std::ostringstream oss{};
  if (CHECK_EVENT(_epoll_event, EPOLLIN) != 0) {
    oss << "IN ";
  }
  if (CHECK_EVENT(_epoll_event, EPOLLPRI) != 0) {
    oss << "PRI ";
  }
  if (CHECK_EVENT(_epoll_event, EPOLLOUT) != 0) {
    oss << "OUT ";
  }
  if (CHECK_EVENT(_epoll_event, EPOLLHUP) != 0) {
    oss << "HUP ";
  }
  if (CHECK_EVENT(_epoll_event, EPOLLRDHUP) != 0) {
    oss << "RDHUP ";
  }
  if (CHECK_EVENT(_epoll_event, EPOLLERR) != 0) {
    oss << "ERR ";
  }
  return oss.str();
}

}  // namespace io_inner

ReactorEpoll::ReactorEpoll()
    : Reactor(Cycle::REACTOR_TYPE_EPOLL),
      epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
      epoll_events_(io_inner::kInitEventsCnt) {
  if (epoll_fd_ < 0) {
    HARE_INTERNAL_FATAL("cannot create a epoll fd.");
  }
}

ReactorEpoll::~ReactorEpoll() { ::close(epoll_fd_); }

auto ReactorEpoll::Poll(std::int32_t _timeout_microseconds, EventsList& _events)
    -> Timestamp {
  HARE_INTERNAL_TRACE("active events total count: {}.", _events.size());

  auto event_num = ::epoll_wait(epoll_fd_, &*epoll_events_.begin(),
                                static_cast<std::int32_t>(epoll_events_.size()),
                                _timeout_microseconds / 1000);

  auto saved_errno = errno;
  auto now{Timestamp::Now()};
  if (event_num > 0) {
    HARE_INTERNAL_TRACE("{} events happened.", event_num);
    FillActiveEvents(event_num, _events);
    if (ImplicitCast<std::size_t>(event_num) == epoll_events_.size()) {
      epoll_events_.resize(epoll_events_.size() * 2);
    }
  } else if (event_num == 0) {
    HARE_INTERNAL_TRACE("nothing happened.");
  } else {
    // error happens, log uncommon ones
    if (saved_errno != EINTR) {
      errno = saved_errno;
      HARE_INTERNAL_ERROR("there was an error in the reactor.");
    }
  }
  return now;
}

auto ReactorEpoll::EventUpdate(Event* _event) -> bool {
  auto event_id = _event->id();
  HARE_INTERNAL_TRACE("epoll-update: fd={}, flags={}.", _event->fd(),
                      _event->events());

  if (event_id == 0) {
    // a new one, add with EPOLL_CTL_ADD
    auto target_fd = _event->fd();
    IgnoreUnused(target_fd);
    return UpdateEpoll(EPOLL_CTL_ADD, _event);
  }

  // update existing one with EPOLL_CTL_MOD/DEL
  HARE_ASSERT(Check(_event));
  return UpdateEpoll(EPOLL_CTL_MOD, _event);
}

auto ReactorEpoll::EventRemove(Event* _event) -> bool {
  const auto target_fd = _event->fd();
  const auto event_id = _event->id();
  IgnoreUnused(target_fd, event_id);

  HARE_INTERNAL_TRACE("epoll-remove: fd={}, flags={}.", target_fd,
                      _event->events());
  HARE_ASSERT(event_id == 0);

  return UpdateEpoll(EPOLL_CTL_DEL, _event);
}

void ReactorEpoll::FillActiveEvents(std::int32_t _num_of_events,
                                    EventsList& _events) {
  HARE_ASSERT(ImplicitCast<std::size_t>(_num_of_events) <=
              epoll_events_.size());
  for (auto i = 0; i < _num_of_events; ++i) {
    auto* event = static_cast<Event*>(epoll_events_[i].data.ptr);
    Event::Id event_id = epoll_events_[i].data.u64;
    _events.emplace_back(event_id, event,
                         io_inner::EncodeEpoll(epoll_events_[i].events));
  }
}

auto ReactorEpoll::UpdateEpoll(std::int32_t _operation, Event* _event) const
    -> bool {
  struct epoll_event ep_event {};
  ::hare::detail::FillN(&ep_event, sizeof(ep_event), 0);
  ep_event.events = io_inner::DecodeEpoll(_event->events());
  ep_event.data.ptr = _event;
  ep_event.data.u64 = _event->id();
  auto target_fd = _event->fd();

  HARE_INTERNAL_TRACE(
      "epoll_ctl op={} fd={} event=[{}].",
      io_inner::OperationToString(_operation), target_fd,
      io_inner::EpollToString(io_inner::DecodeEpoll(_event->events())));

  if (::epoll_ctl(epoll_fd_, _operation, target_fd, &ep_event) < 0) {
    HARE_INTERNAL_ERROR("epoll_ctl error op = {} fd = {}",
                        io_inner::OperationToString(_operation), target_fd);
    return false;
  }
  return true;
}

}  // namespace hare

#endif  // HARE__HAVE_EPOLL