#ifndef _HARE_NET_SOCKET_OP_H_
#define _HARE_NET_SOCKET_OP_H_

#include <hare/net/util.h>

namespace hare {
namespace socket {

    extern socket_t createNonblockingOrDie(int8_t family);
    
    extern ssize_t write(socket_t fd, const void* buf, size_t count);
    extern ssize_t read(socket_t fd, const void* buf, size_t count);

} // namespace socket
} // namespace hare

#endif // !_HARE_NET_SOCKET_OP_H_