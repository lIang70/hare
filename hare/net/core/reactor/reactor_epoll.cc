#include "hare/net/core/reactor/reactor_epoll.h"
#include "hare/net/core/event.h"
#include <hare/base/logging.h>

#include <sstream>

#ifdef HARE__HAVE_EPOLL

#ifdef H_OS_LINUX
#include <unistd.h>
#endif

namespace hare {
namespace core {

    namespace detail {
        const int32_t INIT_EVENTS_CNT = 16;

        std::string operationToString(int32_t op)
        {
            switch (op) {
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

        decltype(epoll_event::events) decode(int32_t event_flags)
        {
            decltype(epoll_event::events) events = 0;
            if (event_flags & EV_READ)
                events |= (EPOLLIN | EPOLLPRI);
            if (event_flags & EV_WRITE)
                events |= (EPOLLOUT);
            if (event_flags & EV_ET)
                events |= (EPOLLET);
            return events;
        }

        int32_t encode(decltype(epoll_event::events) events)
        {
            int32_t flags = EV_DEFAULT;
            if (events & EPOLLERR) {
                flags = EV_READ | EV_WRITE | EV_ERROR;
            } else if ((events & EPOLLHUP) && !(events & EPOLLRDHUP)) {
                flags = EV_READ | EV_WRITE;
            } else {
                if (events & EPOLLIN)
                    flags |= EV_READ;
                if (events & EPOLLOUT)
                    flags |= EV_WRITE;
                if (events & EPOLLRDHUP)
                    flags |= EV_CLOSED;
            }
            return flags;
        }

        std::string eventsToString(decltype(epoll_event::events) event)
        {
            std::ostringstream oss {};
            if (event & EPOLLIN)
                oss << "IN ";
            if (event & EPOLLPRI)
                oss << "PRI ";
            if (event & EPOLLOUT)
                oss << "OUT ";
            if (event & EPOLLHUP)
                oss << "HUP ";
            if (event & EPOLLRDHUP)
                oss << "RDHUP ";
            if (event & EPOLLERR)
                oss << "ERR ";
            return oss.str();
        }

    } // namespace detail

    EpollReactor::EpollReactor(Cycle* cycle)
        : Reactor(cycle)
        , epoll_fd_(::epoll_create1(EPOLL_CLOEXEC))
        , epoll_events_(detail::INIT_EVENTS_CNT)
    {
        if (epoll_fd_ < 0) {
            SYS_FATAL() << "Cannot create a epoll socket.";
        }
    }

    EpollReactor::~EpollReactor()
    {
        ::close(epoll_fd_);
    }

    Timestamp EpollReactor::poll(int32_t timeout_microseconds, Cycle::EventList& active_events)
    {
        LOG_TRACE() << "Active events total count: " << active_events.size();

        auto event_num = ::epoll_wait(epoll_fd_,
            &*epoll_events_.begin(), static_cast<int32_t>(epoll_events_.size()),
            timeout_microseconds);

        auto saved_errno = errno;
        Timestamp now(Timestamp::now());
        if (event_num > 0) {
            LOG_TRACE() << event_num << " events happened.";
            fillActiveEvents(event_num, active_events);
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

    void EpollReactor::updateEvent(Event* event)
    {
        Reactor::assertInCycleThread();
        auto status = event->status();
        LOG_TRACE() << "fd = " << event->fd()
                    << " flags = " << event->flags() << " status = " << static_cast<int8_t>(status);
        if (status == Event::Status::NEW || status == Event::Status::DELETE) {
            // a new one, add with EPOLL_CTL_ADD
            auto fd = event->fd();
            if (status == Event::Status::NEW) {
                HARE_ASSERT(events_.find(fd) == events_.end(), "Cannot find event.");
                events_[fd] = event;
            } else { // status == Event::Status::DELETE
                HARE_ASSERT(events_.find(fd) != events_.end(), "Event is incorrect.");
                HARE_ASSERT(events_[fd] == event, "Event is incorrect.");
            }

            event->setStatus(Event::Status::ADD);
            update(EPOLL_CTL_ADD, event);
        } else {
            // update existing one with EPOLL_CTL_MOD/DEL
            auto fd = event->fd();
            HARE_ASSERT(events_.find(fd) != events_.end(), "Cannot find event.");
            HARE_ASSERT(events_[fd] == event, "Event is incorrect.");
            HARE_ASSERT(status == Event::Status::ADD, "Event is incorrect.");
            H_UNUSED(fd);
            if (event->isNoneEvent()) {
                event->setStatus(Event::Status::DELETE);
                update(EPOLL_CTL_DEL, event);
            } else {
                update(EPOLL_CTL_MOD, event);
            }
        }
    }

    void EpollReactor::removeEvent(Event* event)
    {
        Reactor::assertInCycleThread();
        auto fd = event->fd();
        LOG_TRACE() << "fd = " << fd;
        HARE_ASSERT(events_.find(fd) != events_.end(), "Cannot find event.");
        HARE_ASSERT(events_[fd] == event, "Event is incorrect.");
        HARE_ASSERT(event->isNoneEvent(), "Event is incorrect.");
        auto status = event->status();
        HARE_ASSERT(status == Event::Status::ADD || status == Event::Status::DELETE, "Incorrect status.");
        auto n = events_.erase(fd);
        HARE_ASSERT(n == 1, "Erase error.");

        if (status == Event::Status::ADD) {
            update(EPOLL_CTL_DEL, event);
        }
        event->setStatus(Event::Status::NEW);
    }

    void EpollReactor::fillActiveEvents(int32_t num_of_events, Cycle::EventList& active_events)
    {
        HARE_ASSERT(implicit_cast<size_t>(num_of_events) <= epoll_events_.size(), "Oversize");
        for (auto i = 0; i < num_of_events; ++i) {
            auto event = static_cast<Event*>(epoll_events_[i].data.ptr);
            auto e = events_[event->fd()];
#ifdef HARE_DEBUG
            auto fd = event->fd();
            auto it = events_.find(fd);
            HARE_ASSERT(it != events_.end(), "Cannot find event.");
            HARE_ASSERT(it->second == e, "Event is incorrect.");
#endif
            event->setRFlags(detail::encode(epoll_events_[i].events));
            active_events.push_back(e);

            // Remove non-persist.
            if (!(event->flags() & EV_PERSIST)) {
                removeEvent(e);
            }
        }
    }

    void EpollReactor::update(int32_t operation, Event* event)
    {
        struct epoll_event ep_event;
        setZero(&ep_event, sizeof(ep_event));
        ep_event.events = detail::decode(event->flags());
        ep_event.data.ptr = event;
        auto fd = event->fd();
        LOG_TRACE() << "epoll_ctl op = " << detail::operationToString(operation)
                    << "\n fd = " << fd << " event = { " << detail::eventsToString(detail::decode(event->flags())) << " }";
        if (::epoll_ctl(epoll_fd_, operation, fd, &ep_event) < 0) {
            if (operation == EPOLL_CTL_DEL) {
                SYS_ERROR() << "epoll_ctl op =" << detail::operationToString(operation) << " fd =" << fd;
            } else {
                SYS_FATAL() << "epoll_ctl op =" << detail::operationToString(operation) << " fd =" << fd;
            }
        }
    }

} // namespace core
} // namespace hare

#endif // HARE__HAVE_EPOLL