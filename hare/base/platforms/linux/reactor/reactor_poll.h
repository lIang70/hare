#ifndef _HARE_BASE_IO_REACTOR_POLL_H_
#define _HARE_BASE_IO_REACTOR_POLL_H_

#include <hare/hare-config.h>

#include <map>
#include <vector>

#include "base/io/reactor.h"

#if HARE__HAVE_POLL
#include <sys/poll.h>

namespace hare {

class ReactorPoll : public Reactor {
  using pollfd_list = std::vector<struct pollfd>;

  pollfd_list poll_fds_{};

  struct PollData {
    Event::Id id;
    std::size_t index;
  };
  std::map<util_socket_t, PollData> inverse_map_{};

 public:
  explicit ReactorPoll();
  ~ReactorPoll() override;

  auto Poll(std::int32_t _timeout_microseconds, EventsList& _events)
      -> Timestamp override;
  auto EventUpdate(Event* _event) -> bool override;
  auto EventRemove(Event* _event) -> bool override;

 private:
  void FillActiveEvents(std::int32_t _num_of_events, EventsList& _events);
};

}  // namespace hare

#endif  // HARE__HAVE_POLL

#endif  // _HARE_BASE_IO_REACTOR_POLL_H_