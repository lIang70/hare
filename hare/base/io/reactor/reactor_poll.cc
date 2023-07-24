#include "hare/base/fwd-inl.h"
#include "hare/base/io/reactor/reactor_poll.h"

#include <algorithm>
#include <sstream>

#if HARE__HAVE_POLL

namespace hare {
namespace io {

    namespace detail {
        static const std::int32_t INIT_EVENTS_CNT = 16;

        auto decode_poll(std::uint8_t events) -> decltype(pollfd::events)
        {
            decltype(pollfd::events) res { 0 };
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

        auto encode_poll(decltype(pollfd::events) events) -> std::int32_t
        {
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

        auto events_to_string(decltype(pollfd::events) event) -> std::string
        {
            std::ostringstream oss {};
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

    } // namespace detail

    reactor_poll::reactor_poll(cycle* _cycle)
        : reactor(_cycle, cycle::REACTOR_TYPE_POLL)
    {
    }

    reactor_poll::~reactor_poll() = default;

    auto reactor_poll::poll(std::int32_t _timeout_microseconds) -> timestamp
    {
        auto event_num = ::poll(&*poll_fds_.begin(), poll_fds_.size(), _timeout_microseconds / 1000);
        auto saved_errno = errno;
        auto now { timestamp::now() };

        if (event_num > 0) {
            MSG_TRACE("{} events happened.", event_num);
            fill_active_events(event_num);
        } else if (event_num == 0) {
            MSG_TRACE("nothing happened.");
        } else {
            if (saved_errno != EINTR) {
                errno = saved_errno;
                MSG_ERROR("reactor_poll::poll() error.");
            }
        }
        return now;
    }

    auto reactor_poll::event_update(const ptr<event>& _event) -> bool
    {
        MSG_TRACE("poll-update: fd={}, events={}.", _event->fd(), _event->events());

        auto target_fd = _event->fd();
        auto inverse_iter = inverse_map_.find(target_fd);

        if (_event->event_id() == -1) {
            // a new one, add to pollfd_list
            assert(inverse_map_.find(target_fd) == inverse_map_.end());
            assert(inverse_iter == inverse_map_.end());
            struct pollfd poll_fd { };
            hare::detail::fill_n(&poll_fd, sizeof(poll_fd), 0);
            poll_fd.fd = target_fd;
            poll_fd.events = detail::decode_poll(_event->events());
            poll_fd.revents = 0;
            poll_fds_.push_back(poll_fd);
            auto index = static_cast<std::int32_t>(poll_fds_.size()) - 1;
            inverse_map_.emplace(target_fd, index);
            return true;
        }

        // update existing one
        auto event_id = _event->event_id();
        assert(events_.find(event_id) != events_.end());
        assert(events_[event_id] == _event);
        assert(inverse_iter != inverse_map_.end());
        auto& index = inverse_iter->second;
        assert(0 <= index && index < static_cast<std::int32_t>(poll_fds_.size()));
        struct pollfd& pfd = poll_fds_[index];
        assert(pfd.fd == target_fd || pfd.fd == -target_fd - 1);
        pfd.fd = target_fd;
        pfd.events = detail::decode_poll(_event->events());
        pfd.revents = 0;
        return true;
    }

    auto reactor_poll::event_remove(const ptr<event>& _event) -> bool
    {
        const auto target_fd = _event->fd();
        auto inverse_iter = inverse_map_.find(target_fd);
        assert(inverse_iter != inverse_map_.end());
        auto& index = inverse_iter->second;

        MSG_TRACE("poll-remove: fd={}, events={}.", target_fd, _event->events());
        assert(0 <= index && index < static_cast<std::int32_t>(poll_fds_.size()));
        
        const auto& pfd = poll_fds_[index];
        assert(pfd.events == detail::decode_poll(_event->events()));
        if (implicit_cast<std::size_t>(index) == poll_fds_.size() - 1) {
            poll_fds_.pop_back();
        } else {
            // modify the id of event.
            auto& swap_pfd = poll_fds_.back();
            auto swap_fd = swap_pfd.fd;
            assert(inverse_map_.find(swap_fd) != inverse_map_.end());
            inverse_map_[swap_fd] = index;
            std::iter_swap(poll_fds_.begin() + index, poll_fds_.end() - 1);
            poll_fds_.pop_back();
        }
        inverse_map_.erase(inverse_iter);
        return true;
    }

    void reactor_poll::fill_active_events(std::int32_t _num_of_events)
    {
        const event::id size = static_cast<std::uint32_t>(poll_fds_.size());
        for (event::id event_id = 0; event_id < size && _num_of_events > 0; ++event_id) {
            const auto& pfd = poll_fds_[event_id];
            if (pfd.revents > 0) {
                --_num_of_events;
                auto event_iter = events_.find(event_id);
                assert(event_iter != events_.end());
                assert(event_iter->second->fd() == pfd.fd);
                active_events_.emplace_back(event_iter->second, detail::encode_poll(pfd.revents));
            }
        }
    }

} // namespace io
} // namespace hare

#endif // HARE__HAVE_POLL
