#include "hare/base/io/reactor/reactor_epoll.h"
#include "base/io/reactor.h"
#include <cstdint>
#include <hare/base/logging.h>

#include <sstream>

#ifdef HARE__HAVE_EPOLL

#ifdef H_OS_LINUX
#include <unistd.h>
#endif

namespace hare {
namespace io {

    namespace detail {
        const int32_t INIT_EVENTS_CNT = 16;

        auto operation_to_string(int32_t _op) -> std::string
        {
            switch (_op) {
            case EPOLL_CTL_ADD:
                return "ADD";
            case EPOLL_CTL_DEL:
                return "DEL";
            case EPOLL_CTL_MOD:
                return "MOD";
            default:
                HARE_ASSERT(false, "ERROR op for EPOLL");
                return "Unknown Operation";
            }
        }

        auto decode_epoll(uint8_t _events) -> decltype(epoll_event::events)
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

        auto encode_epoll(decltype(epoll_event::events) _events) -> uint8_t
        {
            uint8_t events { EVENT_DEFAULT };

            if ((CHECK_EVENT(_events, EPOLLERR) != 0) || 
                (CHECK_EVENT(_events, EPOLLHUP) != 0 && CHECK_EVENT(_events, EPOLLRDHUP) == 0)) {
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
        : reactor(cycle)
        , epoll_fd_(::epoll_create1(EPOLL_CLOEXEC))
        , epoll_events_(detail::INIT_EVENTS_CNT)
    {
        if (epoll_fd_ < 0) {
            SYS_FATAL() << "Cannot create a epoll fd.";
        }
    }

    reactor_epoll::~reactor_epoll()
    {
        ::close(epoll_fd_);
    }

    auto reactor_epoll::poll(int32_t _timeout_microseconds) -> timestamp
    {
        LOG_TRACE() << "Active events total count: " << detail::tstorage.active_events.size();

        auto event_num = ::epoll_wait(epoll_fd_,
            &*epoll_events_.begin(), static_cast<int32_t>(epoll_events_.size()),
            _timeout_microseconds);

        auto saved_errno = errno;
        auto now {timestamp::now() };
        if (event_num > 0) {
            LOG_TRACE() << event_num << " events happened.";
            fill_active_events(event_num);
            if (implicit_cast<size_t>(event_num) == epoll_events_.size()) {
                epoll_events_.resize(epoll_events_.size() * 2);
            }
        } else if (event_num == 0) {
            LOG_TRACE() << "nothing happened";
        } else {
            // error happens, log uncommon ones
            if (saved_errno != EINTR) {
                errno = saved_errno;
                SYS_ERROR() << "There was an error in the reactor.";
            }
        }
        return now;
    }

    void reactor_epoll::event_update(ptr<event> _event)
    {
        auto event_id = _event->event_id();
        LOG_TRACE() << "epoll-update: fd=" << _event->fd() << ", flags=" << _event->events();
        if (event_id == -1) {
            // a new one, add with EPOLL_CTL_ADD
            auto target_fd = _event->fd();
            HARE_ASSERT(detail::tstorage.inverse_map.find(target_fd) == detail::tstorage.inverse_map.end(), "Event already inserted in epoll.");
            update(EPOLL_CTL_ADD, _event);
            return ;
        }

        // update existing one with EPOLL_CTL_MOD/DEL
        auto target_fd = _event->fd();
        HARE_ASSERT(detail::tstorage.inverse_map.find(target_fd) != detail::tstorage.inverse_map.end(), "Cannot find event.");
        HARE_ASSERT(detail::tstorage.events.find(event_id) != detail::tstorage.events.end(), "Cannot find event.");
        HARE_ASSERT(detail::tstorage.events[event_id] == _event, "Event is incorrect.");
        update(EPOLL_CTL_MOD, _event);
    }

    void reactor_epoll::event_remove(ptr<event> _event)
    {
        const auto target_fd = _event->fd();
        const auto event_id = _event->event_id();
        LOG_TRACE() << "epoll-remove: fd=" << target_fd << ", flags=" << _event->events();
        HARE_ASSERT(detail::tstorage.inverse_map.find(target_fd) != detail::tstorage.inverse_map.end(), "Cannot find event.");
        HARE_ASSERT(event_id == -1, "Incorrect status.");

        update(EPOLL_CTL_DEL, _event);
    }

    void reactor_epoll::fill_active_events(int32_t _num_of_events)
    {
        HARE_ASSERT(implicit_cast<size_t>(_num_of_events) <= epoll_events_.size(), "Oversize.");
        for (auto i = 0; i < _num_of_events; ++i) {
            auto* event = static_cast<io::event*>(epoll_events_[i].data.ptr);
            HARE_ASSERT(detail::tstorage.inverse_map.find(event->fd()) != detail::tstorage.inverse_map.end(), "Cannot find fd.");
            auto event_id = detail::tstorage.inverse_map[event->fd()];
            HARE_ASSERT(detail::tstorage.events.find(event_id) != detail::tstorage.events.end(), "Cannot find event.");
            auto revent = detail::tstorage.events[event_id];
#ifdef HARE_DEBUG
            HARE_ASSERT(event == revent.get(), "Event is incorrect.");
#endif
            detail::tstorage.active_events.emplace_back(revent, detail::encode_epoll(epoll_events_[i].events));
        }
    }

    void reactor_epoll::update(int32_t _operation, const ptr<event>& _event) const
    {
        struct epoll_event ep_event { };
        set_zero(&ep_event, sizeof(ep_event));
        ep_event.events = detail::decode_epoll(_event->events());
        ep_event.data.ptr = _event.get();
        auto target_fd = _event->fd();
        LOG_TRACE() << "epoll_ctl op = " << detail::operation_to_string(_operation)
                    << "\n fd = " << target_fd << " event = { " << detail::epoll_to_string(detail::decode_epoll(_event->events())) << " }";
        if (::epoll_ctl(epoll_fd_, _operation, target_fd, &ep_event) < 0) {
            if (_operation == EPOLL_CTL_DEL) {
                SYS_ERROR() << "epoll_ctl op =" << detail::operation_to_string(_operation) << " fd =" << target_fd;
            } else {
                SYS_FATAL() << "epoll_ctl op =" << detail::operation_to_string(_operation) << " fd =" << target_fd;
            }
        }
    }

} // namespace io
} // namespace hare

#endif // HARE__HAVE_EPOLL