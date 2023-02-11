#ifndef _HARE_NET_EVENT_DATA_H_
#define _HARE_NET_EVENT_DATA_H_

#include <hare/net/event.h>

#include <sstream>
#ifdef H_OS_WIN32
#else
#include <sys/poll.h>
#endif

namespace hare {

struct Event::Data {
    int8_t event_flags_ { EV_DEFAULT };
    int32_t revents_ { 0 };

    std::string reventsToString()
    {
        std::ostringstream oss {};
#ifdef H_OS_WIN32
#else
        if (revents_ & POLLIN)
            oss << "IN ";
        if (revents_ & POLLPRI)
            oss << "PRI ";
        if (revents_ & POLLOUT)
            oss << "OUT ";
        if (revents_ & POLLHUP)
            oss << "HUP ";
        if (revents_ & POLLRDHUP)
            oss << "RDHUP ";
        if (revents_ & POLLERR)
            oss << "ERR ";
        if (revents_ & POLLNVAL)
            oss << "NVAL ";
        return oss.str();
#endif
    }
};

} // namespace hare

#endif // !_HARE_NET_EVENT_DATA_H_