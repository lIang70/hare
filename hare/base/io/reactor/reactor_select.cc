#include "hare/base/io/reactor/reactor_select.h"
#include "hare/base/io/local.h"
#include <hare/base/exception.h>

#if HARE__HAVE_SELECT

namespace hare {
namespace io {

    namespace detail {

        HARE_INLINE
        static void cast_to(struct timeval* _tv, std::int32_t _microseconds)
        {
            _tv->tv_sec = _microseconds / 1000000;
            _tv->tv_usec = (_microseconds - _tv->tv_sec * 1000000) * 1000;
        }

    } // namespace detail

    reactor_select::reactor_select(cycle* cycle)
        : reactor(cycle, cycle::REACTOR_TYPE_SELECT)
    {
        FD_ZERO(&fds_set_);
    }

    auto reactor_select::poll(std::int32_t _timeout_microseconds) -> timestamp
    {
        auto read_fds = read_set_;
        auto write_fds = write_set_;
        auto except_fds = fds_set_;

        struct timeval tv { };
        detail::cast_to(&tv, _timeout_microseconds);
        auto event_num = ::select(FD_SETSIZE, &read_fds, &write_fds, &except_fds, &tv);

        auto saved_errno = errno;
        auto now { timestamp::now() };
        if (event_num > 0) {
            MSG_TRACE("{} events happened.", event_num);

            for (const auto& event : inverse_map_) {
                auto event_fd = event.first;
                std::uint8_t revents {};
                if (FD_ISSET(event_fd, &read_fds)) {
                    SET_EVENT(revents, EVENT_READ);
                }
                if (FD_ISSET(event_fd, &write_fds)) {
                    SET_EVENT(revents, EVENT_WRITE);
                }
                if (FD_ISSET(event_fd, &except_fds)) {
                    SET_EVENT(revents, EVENT_CLOSED);
                }
                if (revents != EVENT_DEFAULT) {
                    auto iter = events_.find(event.second);
                    if (iter != events_.end()) {
                        active_events_.emplace_back(iter->second, revents);
                    }
                }
            }

        } else if (event_num == 0) {
            MSG_TRACE("nothing happened.");
        } else {
            // error happens, log uncommon ones
            if (saved_errno != EINTR) {
                errno = saved_errno;
                MSG_ERROR("there was an error in the reactor.");
            }
        }
        return now;
    }

    auto reactor_select::event_update(const ptr<event>& _event) -> bool
    {
        if (_event->fd() >= FD_SETSIZE) {
            MSG_ERROR("fd[{}] exceeds the {} limit, so it cannot be added.", _event->fd(), FD_SETSIZE);
            return false;
        }

        FD_SET(_event->fd(), &fds_set_);
        if (CHECK_EVENT(_event->events(), EVENT_READ)) {
            FD_SET(_event->fd(), &read_set_);
        }
        if (CHECK_EVENT(_event->events(), EVENT_WRITE)) {
            FD_SET(_event->fd(), &write_set_);
        }
        return true;
    }

    auto reactor_select::event_remove(const ptr<event>& _event) -> bool
    {
        if (_event->fd() >= FD_SETSIZE) {
            MSG_ERROR("fd[{}] exceeds the {} limit, so it does not exist with reactor.", _event->fd(), FD_SETSIZE);
            return false;
        }

        FD_CLR(_event->fd(), &fds_set_);
        FD_CLR(_event->fd(), &read_set_);
        FD_CLR(_event->fd(), &write_set_);
        return true;
    }

} // namespace io
} // namespace hare

#endif // HARE__HAVE_SELECT