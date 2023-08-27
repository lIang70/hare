#include "base/fwd-inl.h"
#include "base/io/reactor/reactor_poll.h"

#include <algorithm>
#include <sstream>

#if HARE__HAVE_POLL

namespace hare {
namespace io {

    namespace detail {
        static const std::int32_t kInitEventsCnt = 16;

        static auto DecodePoll(std::uint8_t events) -> decltype(pollfd::events)
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

        static auto EncodePoll(decltype(pollfd::events) events) -> std::int32_t
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

        static auto EventsToString(decltype(pollfd::events) event) -> std::string
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

    ReactorPoll::ReactorPoll(Cycle* _cycle)
        : Reactor(_cycle, Cycle::REACTOR_TYPE_POLL)
        , poll_fds_(detail::kInitEventsCnt)
    {
    }

    ReactorPoll::~ReactorPoll() = default;

    auto ReactorPoll::Poll(std::int32_t _timeout_microseconds) -> Timestamp
    {
        auto event_num = ::poll(&*poll_fds_.begin(), poll_fds_.size(), _timeout_microseconds / 1000);
        auto saved_errno = errno;
        auto now { Timestamp::Now() };

        if (event_num > 0) {
            HARE_INTERNAL_TRACE("{} events happened.", event_num);
            FillActiveEvents(event_num);
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

    auto ReactorPoll::EventUpdate(const Ptr<Event>& _event) -> bool
    {
        HARE_INTERNAL_TRACE("poll-update: fd={}, events={}.", _event->fd(), _event->events());

        auto target_fd = _event->fd();
        auto inverse_iter = inverse_map_.find(target_fd);

        if (_event->id() == -1) {
            // a new one, add to pollfd_list
            HARE_ASSERT(inverse_map_.find(target_fd) == inverse_map_.end());
            HARE_ASSERT(inverse_iter == inverse_map_.end());
            struct pollfd poll_fd { };
            hare::detail::FillN(&poll_fd, sizeof(poll_fd), 0);
            poll_fd.fd = target_fd;
            poll_fd.events = detail::DecodePoll(_event->events());
            poll_fd.revents = 0;
            poll_fds_.push_back(poll_fd);
            auto index = static_cast<std::int32_t>(poll_fds_.size()) - 1;
            inverse_map_.emplace(target_fd, index);
            return true;
        }

        // update existing one
        auto event_id = _event->id();
        IgnoreUnused(event_id);
        HARE_ASSERT(events_.find(event_id) != events_.end());
        HARE_ASSERT(events_[event_id] == _event);
        HARE_ASSERT(inverse_iter != inverse_map_.end());
        auto& index = inverse_iter->second;
        HARE_ASSERT(0 <= index && index < static_cast<std::int32_t>(poll_fds_.size()));
        struct pollfd& pfd = poll_fds_[index];
        HARE_ASSERT(pfd.fd == target_fd || pfd.fd == -target_fd - 1);
        pfd.fd = target_fd;
        pfd.events = detail::DecodePoll(_event->events());
        pfd.revents = 0;
        return true;
    }

    auto ReactorPoll::EventRemove(const Ptr<Event>& _event) -> bool
    {
        const auto target_fd = _event->fd();
        auto inverse_iter = inverse_map_.find(target_fd);
        HARE_ASSERT(inverse_iter != inverse_map_.end());
        auto& index = inverse_iter->second;

        HARE_INTERNAL_TRACE("poll-remove: fd={}, events={}.", target_fd, _event->events());
        HARE_ASSERT(0 <= index && index < static_cast<std::int32_t>(poll_fds_.size()));
        
        const auto& pfd = poll_fds_[index];
        IgnoreUnused(pfd);
        HARE_ASSERT(pfd.events == detail::DecodePoll(_event->events()));
        if (ImplicitCast<std::size_t>(index) == poll_fds_.size() - 1) {
            poll_fds_.pop_back();
        } else {
            // modify the id of event.
            auto& swap_pfd = poll_fds_.back();
            auto swap_fd = swap_pfd.fd;
            HARE_ASSERT(inverse_map_.find(swap_fd) != inverse_map_.end());
            inverse_map_[swap_fd] = index;
            std::iter_swap(poll_fds_.begin() + index, poll_fds_.end() - 1);
            poll_fds_.pop_back();
        }
        inverse_map_.erase(inverse_iter);
        return true;
    }

    void ReactorPoll::FillActiveEvents(std::int32_t _num_of_events)
    {
        const Event::Id size = static_cast<std::uint32_t>(poll_fds_.size());
        for (Event::Id event_id = 0; event_id < size && _num_of_events > 0; ++event_id) {
            const auto& pfd = poll_fds_[event_id];
            if (pfd.revents > 0) {
                --_num_of_events;
                auto event_iter = events_.find(event_id);
                HARE_ASSERT(event_iter != events_.end());
                HARE_ASSERT(event_iter->second->fd() == pfd.fd);
                active_events_.emplace_back(event_iter->second, detail::EncodePoll(pfd.revents));
            }
        }
    }

} // namespace io
} // namespace hare

#endif // HARE__HAVE_POLL
