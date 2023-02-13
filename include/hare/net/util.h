#ifndef _HARE_NET_UTIL_H_
#define _HARE_NET_UTIL_H_

#include <hare/base/util.h>

namespace hare {

#ifdef H_OS_WIN32
using socket_t = intptr_t;
#else
using socket_t = int;
#endif

enum : int32_t {
    //! @brief The default flag.
    EV_DEFAULT = 0x00,
    //! @brief Indicates that a timeout has occurred.
    //!  It's not necessary to pass this flag to Event.
    EV_TIMEOUT = 0x01,
    //! @brief Wait for a socket or FD to become readable.
    EV_READ = 0x02,
    //! @brief Wait for a socket or FD to become writeable.
    EV_WRITE = 0x04,
    //! @brief Select edge-triggered behavior, if supported by the backend.
    EV_ET = 0x10,
    //! @brief Detects connection close events. You can use this to detect when a
    //!  connection has been closed, without having to read all the pending data
    //!  from a connection.
    EV_CLOSED = 0x20,
    EV_ERROR = 0x30
};

} // namespace hare

#endif // !_HARE_NET_UTIL_H_