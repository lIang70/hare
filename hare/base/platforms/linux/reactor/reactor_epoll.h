#ifndef _HARE_BASE_IO_REACTOR_EPOLL_H_
#define _HARE_BASE_IO_REACTOR_EPOLL_H_

#include <hare/hare-config.h>

#include <vector>

#include "base/io/reactor.h"

#if HARE__HAVE_EPOLL
#include <sys/epoll.h>

namespace hare {

class ReactorEpoll : public Reactor {
  using ep_event_list = std::vector<struct epoll_event>;

  util_socket_t epoll_fd_{-1};
  ep_event_list epoll_events_{};

 public:
  explicit ReactorEpoll();
  ~ReactorEpoll() override;

  auto Poll(std::int32_t _timeout_microseconds, EventsList& _events)
      -> Timestamp override;
  auto EventUpdate(Event* _event) -> bool override;
  auto EventRemove(Event* _event) -> bool override;

 private:
  void FillActiveEvents(std::int32_t _num_of_events, EventsList& _events);
  auto UpdateEpoll(std::int32_t _operation, Event* _event) const -> bool;
};

}  // namespace hare

#endif  // HARE__HAVE_EPOLL

#endif  // _HARE_BASE_IO_REACTOR_EPOLL_H_