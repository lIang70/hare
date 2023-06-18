#include "hare/base/io/reactor/reactor_poll.h"

#include "hare/base/thread/local.h"
#include <hare/base/logging.h>

#include <algorithm>
#include <sstream>

#if HARE__HAVE_POLL

namespace hare {
namespace io {

    namespace detail {
        static const int32_t INIT_EVENTS_CNT = 16;

        auto decode_poll(uint8_t events) -> decltype(pollfd::events)
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

        auto encode_poll(decltype(pollfd::events) events) -> int32_t
        {
            uint8_t res = EVENT_DEFAULT;
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

    reactor_poll::reactor_poll(cycle* _cycle, cycle::REACTOR_TYPE _type)
        : reactor(_cycle, _type)
    {
    }

    reactor_poll::~reactor_poll() = default;

    auto reactor_poll::poll(int32_t _timeout_microseconds) -> timestamp
    {
        auto event_num = ::poll(&*poll_fds_.begin(), poll_fds_.size(), _timeout_microseconds);
        auto saved_errno = errno;
        auto now { timestamp::now() };

        if (event_num > 0) {
            LOG_TRACE() << event_num << " events happened";
            fill_active_events(event_num);
        } else if (event_num == 0) {
            LOG_TRACE() << "nothing happened";
        } else {
            if (saved_errno != EINTR) {
                errno = saved_errno;
                SYS_ERROR() << "reactor_poll::poll()";
            }
        }
        return now;
    }

    void reactor_poll::event_update(ptr<event> _event)
    {
        LOG_TRACE() << "poll-update: fd=" << _event->fd() << ", events=" << _event->events();
        auto target_fd = _event->fd();
        auto inverse_iter = inverse_map_.find(target_fd);
        
        if (_event->event_id() == -1) {
            // a new one, add to pollfd_list
            HARE_ASSERT(current_thread::get_tds().inverse_map.find(target_fd) == current_thread::get_tds().inverse_map.end(), "error in event.");
            HARE_ASSERT(inverse_iter == inverse_map_.end(), "the fd already inserted into poll.");
            struct pollfd poll_fd {};
            set_zero(&poll_fd, sizeof(poll_fd));
            poll_fd.fd = target_fd;
            poll_fd.events = detail::decode_poll(_event->events());
            poll_fd.revents = 0;
            poll_fds_.push_back(poll_fd);
            auto index = static_cast<int32_t>(poll_fds_.size()) - 1;
            inverse_map_.emplace(target_fd, index); 
            return ;
        }

        // update existing one
        auto event_id = _event->event_id();
        HARE_ASSERT(current_thread::get_tds().events.find(event_id) != current_thread::get_tds().events.end(), "cannot find event.");
        HARE_ASSERT(current_thread::get_tds().events[event_id] == _event, "event is incorrect.");
        HARE_ASSERT(inverse_iter != inverse_map_.end(), "the fd doesn't exist in poll.");
        auto& index = inverse_iter->second;
        HARE_ASSERT(0 <= index && index < static_cast<int32_t>(poll_fds_.size()), "oversize.");
        struct pollfd& pfd = poll_fds_[index];
        HARE_ASSERT(pfd.fd == target_fd || pfd.fd == -target_fd - 1, "error in event.");
        pfd.fd = target_fd;
        pfd.events = detail::decode_poll(_event->events());
        pfd.revents = 0;
    }

    void reactor_poll::event_remove(ptr<event> _event)
    {
        const auto target_fd = _event->fd();

        auto inverse_iter = inverse_map_.find(target_fd);
        HARE_ASSERT(inverse_iter != inverse_map_.end(), "the fd doesn't exist in poll.");
        auto& index = inverse_iter->second;
        LOG_TRACE() << "poll-remove: fd=" << target_fd;
        HARE_ASSERT(0 <= index && index < static_cast<int32_t>(poll_fds_.size()), "oversize.");
        const auto& pfd = poll_fds_[index];
        HARE_ASSERT(pfd.events == detail::decode_poll(_event->events()), "error events.");
        if (implicit_cast<size_t>(index) == poll_fds_.size() - 1) {
            poll_fds_.pop_back();
        } else {
            // modify the id of event.
            auto& swap_pfd = poll_fds_.back();
            auto swap_fd = swap_pfd.fd;
            HARE_ASSERT(inverse_map_.find(swap_fd) != inverse_map_.end(), "the fd doesn't exist in poll.");
            inverse_map_[swap_fd] = index;
            std::iter_swap(poll_fds_.begin() + index, poll_fds_.end() - 1);
            poll_fds_.pop_back();
        }
        inverse_map_.erase(inverse_iter);
    }

    void reactor_poll::fill_active_events(int32_t _num_of_events)
    {
        const auto size = poll_fds_.size();
        for (auto event_id = 0; event_id < size && _num_of_events > 0; ++event_id) {
            const auto& pfd = poll_fds_[event_id];
            if (pfd.revents > 0) {
                --_num_of_events;
                auto event_iter = current_thread::get_tds().events.find(event_id);
                HARE_ASSERT(event_iter != current_thread::get_tds().events.end(), "the event does not exist in the reactor.");
                HARE_ASSERT(event_iter->second->fd() == pfd.fd, "the event's fd does not match.");
                current_thread::get_tds().active_events.emplace_back(event_iter->second, detail::encode_poll(pfd.revents));
            }
        }
    }

} // namespace io
} // namespace hare

#endif // HARE__HAVE_POLL
