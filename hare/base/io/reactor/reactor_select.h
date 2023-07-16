#ifndef _HARE_BASE_IO_REACTOR_SELECT_H_
#define _HARE_BASE_IO_REACTOR_SELECT_H_

#include "hare/base/io/reactor.h"
#include <hare/hare-config.h>

#if HARE__HAVE_SELECT
#include <sys/select.h>

namespace hare {
namespace io {

    class reactor_select : public reactor {
        fd_set fds_set_ {};
        fd_set read_set_ {};
        fd_set write_set_ {};

    public:
        explicit reactor_select(cycle* _cycle);
        ~reactor_select() override = default;

        auto poll(std::int32_t _timeout_microseconds) -> timestamp override;
        auto event_update(const ptr<event>& _event) -> bool override;
        auto event_remove(const ptr<event>& _event) -> bool override;
    };

} // namespace io
} // namespace hare

#endif // HARE__HAVE_SELECT

#endif // _HARE_BASE_IO_REACTOR_SELECT_H_