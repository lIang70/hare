#ifndef _HARE_NET_REACTOR_POLL_H_
#define _HARE_NET_REACTOR_POLL_H_

#include "hare/net/core/reactor.h"

#ifdef HARE__HAVE_POLL
#include <sys/poll.h>

namespace hare {
namespace core {

    class PollReactor : public Reactor {
        using PollFdList = std::vector<struct pollfd>;

        PollFdList poll_fds_ {};

    public:
        explicit PollReactor(Cycle* cycle);
        ~PollReactor() override;

        Timestamp poll(int32_t timeout_microseconds, Cycle::EventList& active_events) override;
        void updateEvent(Event* event) override;
        void removeEvent(Event* event) override;

    private:
        void fillActiveEvents(int32_t num_of_events, Cycle::EventList& active_events);
    };

} // namespace core
} // namespace hare

#endif // HARE__HAVE_POLL

#endif // !_HARE_NET_REACTOR_POLL_H_