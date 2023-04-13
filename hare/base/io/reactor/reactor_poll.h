#ifndef _HARE_NET_REACTOR_POLL_H_
#define _HARE_NET_REACTOR_POLL_H_

#include "hare/base/io/reactor.h"
#include <hare/hare-config.h>

#ifdef HARE__HAVE_POLL
#include <sys/poll.h>

namespace hare {
namespace io {

    class reactor_poll : public reactor {
        using pollfd_list = std::vector<struct pollfd>;

        pollfd_list poll_fds_ {};

    public:
        explicit reactor_poll(cycle* _cycle);
        ~reactor_poll() override;

        auto poll(int32_t _timeout_microseconds, cycle::event_list& _active_events) -> timestamp override;
        void event_update(ptr<event> _event) override;
        void event_remove(ptr<event> _event) override;

    private:
        void fill_active_events(int32_t num_of_events, cycle::event_list& active_events);
    };

} // namespace io
} // namespace hare

#endif // HARE__HAVE_POLL

#endif // !_HARE_NET_REACTOR_POLL_H_