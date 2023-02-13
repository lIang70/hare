#include "hare/net/core/reactor/reactor_poll.h"
#include "hare/net/core/event.h"
#include <hare/base/logging.h>

#include <sstream>
#include <algorithm>

#ifdef HARE__HAVE_POLL

namespace hare {
namespace core {

    namespace detail {
        const int32_t INIT_EVENTS_CNT = 16;

        decltype(pollfd::events) decodePoll(int32_t event_flags)
        {
            decltype(pollfd::events) events = 0;
            if (event_flags & EV_READ)
                events |= (POLLIN | POLLPRI);
            if (event_flags & EV_WRITE)
                events |= (POLLOUT);
            return events;
        }

        int32_t encodePoll(decltype(pollfd::events) events)
        {
            int32_t flags = EV_DEFAULT;
            if (events & POLLERR) {
                flags = EV_READ | EV_WRITE;
            } else if ((events & POLLHUP) && !(events & POLLRDHUP)) {
                flags = EV_READ | EV_WRITE;
            } else {
                if (events & POLLIN)
                    flags |= EV_READ;
                if (events & POLLOUT)
                    flags |= EV_WRITE;
                if (events & POLLRDHUP)
                    flags |= EV_CLOSED;
            }
            return flags;
        }

        std::string eventsToString(decltype(pollfd::events) event)
        {
            std::ostringstream oss {};
            if (event & POLLIN)
                oss << "IN ";
            if (event & POLLPRI)
                oss << "PRI ";
            if (event & POLLOUT)
                oss << "OUT ";
            if (event & POLLHUP)
                oss << "HUP ";
            if (event & POLLRDHUP)
                oss << "RDHUP ";
            if (event & POLLERR)
                oss << "ERR ";
            return oss.str();
        }

    } // namespace detail

    PollReactor::PollReactor(Cycle* cycle)
        : Reactor(cycle)
    {
    }

    PollReactor::~PollReactor() = default;

    Timestamp PollReactor::poll(int32_t timeout_microseconds, Cycle::EventList& active_events)
    {
        auto event_num = ::poll(&*poll_fds_.begin(), poll_fds_.size(), timeout_microseconds);
        auto saved_errno = errno;
        Timestamp now(Timestamp::now());

        if (event_num > 0) {
            LOG_TRACE() << event_num << " events happened";
            fillActiveEvents(event_num, active_events);
        } else if (event_num == 0) {
            LOG_TRACE() << " nothing happened";
        } else {
            if (saved_errno != EINTR) {
                errno = saved_errno;
                SYS_ERROR() << "PollPoller::poll()";
            }
        }
        return now;
    }

    void PollReactor::updateEvent(Event* event)
    {
        Reactor::assertInCycleThread();
        LOG_TRACE() << "fd = " << event->fd() << " events = " << event->flags();
        auto fd = event->fd();
        if (event->index() == Event::Status::NEW) {
            // a new one, add to pollfds_
            HARE_ASSERT(events_.find(fd) == events_.end(), "Error in event.");
            struct pollfd pfd { };
            setZero(&pfd, sizeof(pfd));
            pfd.fd = fd;
            pfd.events = detail::decodePoll(event->flags());
            pfd.revents = 0;
            poll_fds_.push_back(pfd);
            auto index = static_cast<int32_t>(poll_fds_.size()) - 1;
            event->setIndex(index);
            events_[pfd.fd] = event;
        } else {
            // update existing one
            HARE_ASSERT(events_.find(fd) != events_.end(), "Cannot find event.");
            HARE_ASSERT(events_[fd] == event, "Event is incorrect.");
            auto index = event->index();
            HARE_ASSERT(0 <= index && index < static_cast<int32_t>(poll_fds_.size()), "Over size.");
            struct pollfd& pfd = poll_fds_[index];
            HARE_ASSERT(pfd.fd == fd || pfd.fd == -fd - 1, "Error in event.");
            pfd.fd = fd;
            pfd.events = detail::decodePoll(event->flags());
            pfd.revents = 0;
            if (event->isNoneEvent()) {
                // ignore this pollfd
                pfd.fd = -fd - 1;
            }
        }
    }

    void PollReactor::removeEvent(Event* event)
    {
        Reactor::assertInCycleThread();
        auto fd = event->fd();
        LOG_TRACE() << "fd = " << fd;
        HARE_ASSERT(events_.find(fd) != events_.end(), "Cannot find event.");
        HARE_ASSERT(events_[fd] == event, "Event is incorrect.");
        HARE_ASSERT(event->isNoneEvent(), "Event is incorrect.");
        int index = event->index();
        HARE_ASSERT(0 <= index && index < static_cast<int32_t>(poll_fds_.size()), "Over size.");
        const struct pollfd& pfd = poll_fds_[index];
        HARE_ASSERT(pfd.fd == -fd - 1 && pfd.events == detail::decodePoll(event->flags()), "Error events.");
        auto n = events_.erase(fd);
        HARE_ASSERT(n == 1, "Error in erase.");
        if (implicit_cast<size_t>(index) == poll_fds_.size() - 1) {
            poll_fds_.pop_back();
        } else {
            auto back_fd = poll_fds_.back().fd;
            std::iter_swap(poll_fds_.begin() + index, poll_fds_.end() - 1);
            if (back_fd < 0) {
                back_fd = -back_fd - 1;
            }
            events_[back_fd]->setIndex(index);
            poll_fds_.pop_back();
        }
    }

    void PollReactor::fillActiveEvents(int32_t num_of_events, Cycle::EventList& active_events)
    {
        for (auto pfd = poll_fds_.begin();
             pfd != poll_fds_.end() && num_of_events > 0;
             ++pfd) {
            if (pfd->revents > 0) {
                --num_of_events;
                auto event_iter = events_.find(pfd->fd);
                HARE_ASSERT(event_iter != events_.end(), "The event does not exist in the reactor.");
                auto event = event_iter->second;
                HARE_ASSERT(event->fd() == pfd->fd, "The event's fd does not match.");
                event->setRFlags(detail::encodePoll(pfd->revents));
                active_events.push_back(event);
            }
        }
    }

} // namespace core
} // namespace hare

#endif // HARE__HAVE_POLL
