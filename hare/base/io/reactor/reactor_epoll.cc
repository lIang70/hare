#include "hare/base/io/reactor/reactor_epoll.h"
#include "hare/base/io/local.h"
#include <hare/base/exception.h>

#include <sstream>

#if HARE__HAVE_EPOLL

#ifdef H_OS_LINUX
#include <unistd.h>
#endif

namespace hare {
namespace io {

    namespace detail {
        const std::int32_t INIT_EVENTS_CNT = 16;

        auto operation_to_string(std::int32_t _op) -> std::string
        {
            switch (_op) {
            case EPOLL_CTL_ADD:
                return "ADD";
            case EPOLL_CTL_DEL:
                return "DEL";
            case EPOLL_CTL_MOD:
                return "MOD";
            default:
                assert(false);
                return "Unknown Operation";
            }
        }

        auto decode_epoll(std::uint8_t _events) -> decltype(epoll_event::events)
        {
            decltype(epoll_event::events) events = 0;
            if (CHECK_EVENT(_events, EVENT_READ) != 0) {
                SET_EVENT(events, EPOLLIN | EPOLLPRI);
            }
            if (CHECK_EVENT(_events, EVENT_WRITE) != 0) {
                SET_EVENT(events, EPOLLOUT);
            }
            if (CHECK_EVENT(_events, EVENT_ET) != 0) {
                SET_EVENT(events, EPOLLET);
            }
            return events;
        }

        auto encode_epoll(decltype(epoll_event::events) _events) -> std::uint8_t
        {
            std::uint8_t events { EVENT_DEFAULT };

            if ((CHECK_EVENT(_events, EPOLLERR) != 0) || (CHECK_EVENT(_events, EPOLLHUP) != 0 && CHECK_EVENT(_events, EPOLLRDHUP) == 0)) {
                SET_EVENT(events, EVENT_READ | EVENT_WRITE);
            } else {
                if (CHECK_EVENT(_events, EPOLLIN) != 0) {
                    SET_EVENT(events, EVENT_READ);
                }
                if (CHECK_EVENT(_events, EPOLLOUT) != 0) {
                    SET_EVENT(events, EVENT_WRITE);
                }
                if (CHECK_EVENT(_events, EPOLLRDHUP) != 0) {
                    SET_EVENT(events, EVENT_CLOSED);
                }
            }

            return events;
        }

        auto epoll_to_string(decltype(epoll_event::events) _epoll_event) -> std::string
        {
            std::ostringstream oss {};
            if (CHECK_EVENT(_epoll_event, EPOLLIN) != 0) {
                oss << "IN ";
            }
            if (CHECK_EVENT(_epoll_event, EPOLLPRI) != 0) {
                oss << "PRI ";
            }
            if (CHECK_EVENT(_epoll_event, EPOLLOUT) != 0) {
                oss << "OUT ";
            }
            if (CHECK_EVENT(_epoll_event, EPOLLHUP) != 0) {
                oss << "HUP ";
            }
            if (CHECK_EVENT(_epoll_event, EPOLLRDHUP) != 0) {
                oss << "RDHUP ";
            }
            if (CHECK_EVENT(_epoll_event, EPOLLERR) != 0) {
                oss << "ERR ";
            }
            return oss.str();
        }

    } // namespace detail

    reactor_epoll::reactor_epoll(cycle* cycle)
        : reactor(cycle, cycle::REACTOR_TYPE_EPOLL)
        , epoll_fd_(::epoll_create1(EPOLL_CLOEXEC))
        , epoll_events_(detail::INIT_EVENTS_CNT)
    {
        if (epoll_fd_ < 0) {
            throw exception("cannot create a epoll fd.");
        }
    }

    reactor_epoll::~reactor_epoll()
    {
        ::close(epoll_fd_);
    }

    auto reactor_epoll::poll(std::int32_t _timeout_microseconds) -> timestamp
    {
        MSG_TRACE("active events total count: {}.", active_events_.size());

        auto event_num = ::epoll_wait(epoll_fd_,
            &*epoll_events_.begin(), static_cast<std::int32_t>(epoll_events_.size()),
            _timeout_microseconds / 1000);

        auto saved_errno = errno;
        auto now { timestamp::now() };
        if (event_num > 0) {
            MSG_TRACE("{} events happened.", event_num);
            fill_active_events(event_num);
            if (implicit_cast<std::size_t>(event_num) == epoll_events_.size()) {
                epoll_events_.resize(epoll_events_.size() * 2);
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

    auto reactor_epoll::event_update(const ptr<event>& _event) -> bool
    {
        auto event_id = _event->event_id();
        MSG_TRACE("epoll-update: fd={}, flags={}.", _event->fd(), _event->events());

        if (event_id == -1) {
            // a new one, add with EPOLL_CTL_ADD
            auto target_fd = _event->fd();
            assert(inverse_map_.find(target_fd) == inverse_map_.end());
            return update(EPOLL_CTL_ADD, _event);
        }

        // update existing one with EPOLL_CTL_MOD/DEL
        auto target_fd = _event->fd();
        assert(inverse_map_.find(target_fd) != inverse_map_.end());
        assert(events_.find(event_id) != events_.end());
        assert(events_[event_id] == _event);
        return update(EPOLL_CTL_MOD, _event);
    }

    auto reactor_epoll::event_remove(const ptr<event>& _event) -> bool
    {
        const auto target_fd = _event->fd();
        const auto event_id = _event->event_id();

        MSG_TRACE("epoll-remove: fd={}, flags={}.", target_fd, _event->events());
        assert(inverse_map_.find(target_fd) != inverse_map_.end());
        assert(event_id == -1);

        return update(EPOLL_CTL_DEL, _event);
    }

    void reactor_epoll::fill_active_events(std::int32_t _num_of_events)
    {
        assert(implicit_cast<std::size_t>(_num_of_events) <= epoll_events_.size());
        for (auto i = 0; i < _num_of_events; ++i) {
            auto* event = static_cast<io::event*>(epoll_events_[i].data.ptr);
            assert(inverse_map_.find(event->fd()) != inverse_map_.end());
            auto event_id = inverse_map_[event->fd()];
            assert(events_.find(event_id) != events_.end());
            auto revent = events_[event_id];
            assert(event == revent.get());
            active_events_.emplace_back(revent, detail::encode_epoll(epoll_events_[i].events));
        }
    }

    auto reactor_epoll::update(std::int32_t _operation, const ptr<event>& _event) const -> bool 
    {
        struct epoll_event ep_event { };
        hare::detail::fill_n(&ep_event, sizeof(ep_event), 0);
        ep_event.events = detail::decode_epoll(_event->events());
        ep_event.data.ptr = _event.get();
        auto target_fd = _event->fd();

        MSG_TRACE("epoll_ctl op={} fd={} event=[{}].",
            detail::operation_to_string(_operation), target_fd, detail::epoll_to_string(detail::decode_epoll(_event->events())));

        if (::epoll_ctl(epoll_fd_, _operation, target_fd, &ep_event) < 0) {
            MSG_ERROR("epoll_ctl error op = {} fd = {}", detail::operation_to_string(_operation), target_fd);
            return false;
        }
        return true;
    }

} // namespace io
} // namespace hare

#endif // HARE__HAVE_EPOLL