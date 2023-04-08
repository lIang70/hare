#ifndef _HARE_NET_UTIL_H_
#define _HARE_NET_UTIL_H_

#include <hare/base/error.h>
#include <hare/base/util.h>

#include <list>

namespace hare {
namespace net {

    using EVENT = enum Event : int32_t {
        //! @brief The default flag.
        EVENT_DEFAULT = 0x00,
        //! @brief Wait for a socket or FD to become readable.
        EVENT_READ = 0x01,
        //! @brief Wait for a socket or FD to become writeable.
        EVENT_WRITE = 0x02,
        //! @brief Select edge-triggered behavior, if supported by the backend.
        EVENT_ET = 0x04,
        //! @brief Detects connection close events. You can use this to detect when a
        //!  connection has been closed, without having to read all the pending data
        //!  from a connection.
        EVENT_CONNECTED = 0x08,
        EVENT_CLOSED = 0x10,
        EVENT_ERROR = 0x20
    };

    HARE_API auto getLocalIp(int32_t type, std::list<std::string>& ip_list) -> Error;

} // namespace net
} // namespace hare

#endif // !_HARE_NET_UTIL_H_