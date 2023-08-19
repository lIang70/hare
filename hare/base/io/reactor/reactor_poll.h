#ifndef _HARE_BASE_IO_REACTOR_POLL_H_
#define _HARE_BASE_IO_REACTOR_POLL_H_

#include "base/io/reactor.h"
#include <hare/hare-config.h>

#include <map>
#include <vector>

#if HARE__HAVE_POLL
#include <sys/poll.h>

namespace hare {
namespace io {

    class ReactorPoll : public Reactor {
        using pollfd_list = std::vector<struct pollfd>;

        pollfd_list poll_fds_ {};
        std::map<util_socket_t, std::int32_t> inverse_map_ {};

    public:
        explicit ReactorPoll(Cycle* _cycle);
        ~ReactorPoll() override;

        auto Poll(std::int32_t _timeout_microseconds) -> Timestamp override;
        auto EventUpdate(const Ptr<Event>& _event) -> bool override;
        auto EventRemove(const Ptr<Event>& _event) -> bool override;

    private:
        void FillActiveEvents(std::int32_t _num_of_events);
    };

} // namespace io
} // namespace hare

#endif // HARE__HAVE_POLL

#endif // _HARE_BASE_IO_REACTOR_POLL_H_