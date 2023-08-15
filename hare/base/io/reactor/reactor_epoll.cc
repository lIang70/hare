#include "hare/base/fwd-inl.h"
#include "hare/base/io/socket_op-inl.h"
#include "hare/base/io/reactor/reactor_epoll.h"
#include <hare/base/exception.h>

#include <sstream>

#if HARE__HAVE_EPOLL && HARE__HAVE_UNISTD_H

#include <unistd.h>

namespace hare {
namespace io {

    namespace detail {
        const std::int32_t kInitEventsCnt = 16;

        auto OperationToString(std::int32_t _op) -> std::string
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

        auto DecodeEpoll(std::uint8_t _events) -> decltype(epoll_event::events)
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

        auto EncodeEpoll(decltype(epoll_event::events) _events) -> std::uint8_t
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

        static auto EpollToString(decltype(epoll_event::events) _epoll_event) -> std::string
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

    ReactorEpoll::ReactorEpoll(Cycle* cycle)
        : Reactor(cycle, Cycle::REACTOR_TYPE_EPOLL)
        , epoll_fd_(::epoll_create1(EPOLL_CLOEXEC))
        , epoll_events_(detail::kInitEventsCnt)
    {
        if (epoll_fd_ < 0) {
            HARE_INTERNAL_FATAL("cannot create a epoll fd.");
        }
    }

    ReactorEpoll::~ReactorEpoll()
    {
        socket_op::Close(epoll_fd_);
    }

    auto ReactorEpoll::Poll(std::int32_t _timeout_microseconds) -> Timestamp
    {
        HARE_INTERNAL_TRACE("active events total count: {}.", active_events_.size());

        auto event_num = ::epoll_wait(epoll_fd_,
            &*epoll_events_.begin(), static_cast<std::int32_t>(epoll_events_.size()),
            _timeout_microseconds / 1000);

        auto saved_errno = errno;
        auto now { Timestamp::Now() };
        if (event_num > 0) {
            HARE_INTERNAL_TRACE("{} events happened.", event_num);
            FillActiveEvents(event_num);
            if (implicit_cast<std::size_t>(event_num) == epoll_events_.size()) {
                epoll_events_.resize(epoll_events_.size() * 2);
            }
        } else if (event_num == 0) {
            HARE_INTERNAL_TRACE("nothing happened.");
        } else {
            // error happens, log uncommon ones
            if (saved_errno != EINTR) {
                errno = saved_errno;
                HARE_INTERNAL_ERROR("there was an error in the reactor.");
            }
        }
        return now;
    }

    auto ReactorEpoll::EventUpdate(const Ptr<Event>& _event) -> bool
    {
        auto event_id = _event->id();
        HARE_INTERNAL_TRACE("epoll-update: fd={}, flags={}.", _event->fd(), _event->events());

        if (event_id == -1) {
            // a new one, add with EPOLL_CTL_ADD
            auto target_fd = _event->fd();
            assert(inverse_map_.find(target_fd) == inverse_map_.end());
            return UpdateEpoll(EPOLL_CTL_ADD, _event);
        }

        // update existing one with EPOLL_CTL_MOD/DEL
        auto target_fd = _event->fd();
        assert(inverse_map_.find(target_fd) != inverse_map_.end());
        assert(events_.find(event_id) != events_.end());
        assert(events_[event_id] == _event);
        return UpdateEpoll(EPOLL_CTL_MOD, _event);
    }

    auto ReactorEpoll::EventRemove(const Ptr<Event>& _event) -> bool
    {
        const auto target_fd = _event->fd();
        const auto event_id = _event->id();

        HARE_INTERNAL_TRACE("epoll-remove: fd={}, flags={}.", target_fd, _event->events());
        assert(inverse_map_.find(target_fd) != inverse_map_.end());
        assert(event_id == -1);

        return UpdateEpoll(EPOLL_CTL_DEL, _event);
    }

    void ReactorEpoll::FillActiveEvents(std::int32_t _num_of_events)
    {
        assert(implicit_cast<std::size_t>(_num_of_events) <= epoll_events_.size());
        for (auto i = 0; i < _num_of_events; ++i) {
            auto* event = static_cast<io::Event*>(epoll_events_[i].data.ptr);
            assert(inverse_map_.find(event->fd()) != inverse_map_.end());
            auto event_id = inverse_map_[event->fd()];
            assert(events_.find(event_id) != events_.end());
            auto revent = events_[event_id];
            assert(event == revent.get());
            active_events_.emplace_back(revent, detail::EncodeEpoll(epoll_events_[i].events));
        }
    }

    auto ReactorEpoll::UpdateEpoll(std::int32_t _operation, const Ptr<Event>& _event) const -> bool 
    {
        struct epoll_event ep_event { };
        hare::detail::FillN(&ep_event, sizeof(ep_event), 0);
        ep_event.events = detail::DecodeEpoll(_event->events());
        ep_event.data.ptr = _event.get();
        auto target_fd = _event->fd();

        HARE_INTERNAL_TRACE("epoll_ctl op={} fd={} event=[{}].",
            detail::OperationToString(_operation), target_fd, detail::EpollToString(detail::DecodeEpoll(_event->events())));

        if (::epoll_ctl(epoll_fd_, _operation, target_fd, &ep_event) < 0) {
            HARE_INTERNAL_ERROR("epoll_ctl error op = {} fd = {}", detail::OperationToString(_operation), target_fd);
            return false;
        }
        return true;
    }

} // namespace io
} // namespace hare

#endif // HARE__HAVE_EPOLL