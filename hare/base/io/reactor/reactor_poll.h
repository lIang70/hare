#ifndef _HARE_BASE_IO_REACTOR_POLL_H_
#define _HARE_BASE_IO_REACTOR_POLL_H_

#include "hare/base/io/reactor.h"
#include <hare/hare-config.h>

#include <map>
#include <vector>

#if HARE__HAVE_POLL
#include <sys/poll.h>

namespace hare {
namespace io {

    class reactor_poll : public reactor {
        using pollfd_list = std::vector<struct pollfd>;

        pollfd_list poll_fds_ {};
        std::map<util_socket_t, std::int32_t> inverse_map_ {};

    public:
        explicit reactor_poll(cycle* _cycle, cycle::REACTOR_TYPE _type);
        ~reactor_poll() override;

        auto poll(std::int32_t _timeout_microseconds) -> timestamp override;
        void event_update(ptr<event> _event) override;
        void event_remove(ptr<event> _event) override;

    private:
        void fill_active_events(std::int32_t _num_of_events);
    };

} // namespace io
} // namespace hare

#endif // HARE__HAVE_POLL

#endif // !_HARE_BASE_IO_REACTOR_POLL_H_