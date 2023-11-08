#include "reactor_poll.h"

#include <algorithm>
#include <sstream>

#include "base/fwd_inl.h"

#if HARE__HAVE_POLL

namespace hare {

namespace io_inner {
static const std::int32_t kInitEventsCnt = 16;

static auto DecodePoll(std::uint8_t events) -> decltype(pollfd::events) {
  decltype(pollfd::events) res{0};
  if (CHECK_EVENT(events, EVENT_WRITE) != 0) {
    SET_EVENT(res, POLLOUT);
  }
  if (CHECK_EVENT(events, EVENT_READ) != 0) {
    SET_EVENT(res, POLLIN);
  }
  if (CHECK_EVENT(events, EVENT_CLOSED) != 0) {
    SET_EVENT(res, POLLRDHUP);
  }
  return res;
}

static auto EncodePoll(decltype(pollfd::events) events) -> std::int32_t {
  std::uint8_t res = EVENT_DEFAULT;
  if (CHECK_EVENT(events, (POLLHUP | POLLERR | POLLNVAL)) != 0) {
    SET_EVENT(events, POLLIN | POLLOUT);
  }
  if (CHECK_EVENT(events, POLLIN) != 0) {
    SET_EVENT(res, EVENT_READ);
  }
  if (CHECK_EVENT(events, POLLOUT) != 0) {
    SET_EVENT(res, EVENT_WRITE);
  }
  if (CHECK_EVENT(events, POLLRDHUP) != 0) {
    SET_EVENT(res, EVENT_CLOSED);
  }
  return res;
}

static auto EventsToString(decltype(pollfd::events) event) -> std::string {
  std::ostringstream oss{};
  if ((event & POLLIN) != 0) {
    oss << "IN ";
  }
  if ((event & POLLPRI) != 0) {
    oss << "PRI ";
  }
  if ((event & POLLOUT) != 0) {
    oss << "OUT ";
  }
  if ((event & POLLHUP) != 0) {
    oss << "HUP ";
  }
  if ((event & POLLRDHUP) != 0) {
    oss << "RDHUP ";
  }
  if ((event & POLLERR) != 0) {
    oss << "ERR ";
  }
  return oss.str();
}

}  // namespace io_inner

ReactorPoll::ReactorPoll()
    : Reactor(Cycle::REACTOR_TYPE_POLL), poll_fds_(io_inner::kInitEventsCnt) {}

ReactorPoll::~ReactorPoll() = default;

auto ReactorPoll::Poll(std::int32_t _timeout_microseconds, EventsList& _events)
    -> Timestamp {
  auto event_num = ::poll(&*poll_fds_.begin(), poll_fds_.size(),
                          _timeout_microseconds / 1000);
  auto saved_errno = errno;
  auto now{Timestamp::Now()};

  if (event_num > 0) {
    HARE_INTERNAL_TRACE("{} events happened.", event_num);
    FillActiveEvents(event_num, _events);
  } else if (event_num == 0) {
    HARE_INTERNAL_TRACE("nothing happened.");
  } else {
    if (saved_errno != EINTR) {
      errno = saved_errno;
      HARE_INTERNAL_ERROR("reactor_poll::poll() error.");
    }
  }
  return now;
}

auto ReactorPoll::EventUpdate(Event* _event) -> bool {
  HARE_INTERNAL_TRACE("poll-update: fd={}, events={}.", _event->fd(),
                      _event->events());

  auto fd = _event->fd();
  auto inverse_iter = inverse_map_.find(fd);

  if (!Check(_event)) {
    // a new one, add to pollfd_list
    HARE_ASSERT(inverse_iter == inverse_map_.end());
    struct pollfd poll_fd {};
    hare::detail::FillN(&poll_fd, sizeof(poll_fd), 0);
    poll_fd.fd = fd;
    poll_fd.events = io_inner::DecodePoll(_event->events());
    poll_fd.revents = 0;
    poll_fds_.push_back(poll_fd);
    auto index = poll_fds_.size() - 1;
    inverse_map_.emplace(fd, PollData{_event->id(), index});
    return true;
  }

  // update existing one
  HARE_ASSERT(Check(_event));
  HARE_ASSERT(inverse_iter != inverse_map_.end());
  auto& index = inverse_iter->second.index;
  HARE_ASSERT(0 <= index && index < poll_fds_.size());
  struct pollfd& pfd = poll_fds_[index];
  HARE_ASSERT(pfd.fd == fd || pfd.fd == -fd - 1);
  pfd.fd = fd;
  pfd.events = io_inner::DecodePoll(_event->events());
  pfd.revents = 0;
  return true;
}

auto ReactorPoll::EventRemove(Event* _event) -> bool {
  const auto target_fd = _event->fd();
  auto inverse_iter = inverse_map_.find(target_fd);
  HARE_ASSERT(inverse_iter != inverse_map_.end());
  auto& index = inverse_iter->second.index;

  HARE_INTERNAL_TRACE("poll-remove: fd={}, events={}.", target_fd,
                      _event->events());
  HARE_ASSERT(0 <= index && index < poll_fds_.size());

  HARE_ASSERT(poll_fds_[index].events ==
              io_inner::DecodePoll(_event->events()));
  if (ImplicitCast<std::size_t>(index) == poll_fds_.size() - 1) {
    poll_fds_.pop_back();
  } else {
    // modify the id of event.
    auto& swap_pfd = poll_fds_.back();
    auto swap_fd = swap_pfd.fd;
    HARE_ASSERT(inverse_map_.find(swap_fd) != inverse_map_.end());
    inverse_map_[swap_fd].index = index;
    std::iter_swap(poll_fds_.begin() + static_cast<long>(index),
                   poll_fds_.end() - 1);
    poll_fds_.pop_back();
  }
  inverse_map_.erase(inverse_iter);
  return true;
}

void ReactorPoll::FillActiveEvents(std::int32_t _num_of_events,
                                   EventsList& _events) {
  auto size = poll_fds_.size();
  for (decltype(size) index = 0; index < size && _num_of_events > 0; ++index) {
    const auto& pfd = poll_fds_[index];
    auto iter = inverse_map_.find(pfd.fd);
    if (pfd.revents > 0 && iter != inverse_map_.end()) {
      --_num_of_events;
      auto event_iter = events_.find(iter->second.id);
      if (event_iter == events_.end()) {
        HARE_INTERNAL_ERROR("cannot find event[{}].", iter->second.id);
        continue;
      } else {
        _events.emplace_back(iter->second.id, event_iter->second.event,
                             io_inner::EncodePoll(pfd.revents));
      }
    }
  }
}

}  // namespace hare

#endif  // HARE__HAVE_POLL
