#ifndef _HARE_NET_SOCKET_OP_H_
#define _HARE_NET_SOCKET_OP_H_

#include <hare/net/util.h>

namespace hare {
namespace socket {

    extern socket_t createNonblockingOrDie(int8_t family);
    
    extern int32_t close(socket_t fd);
    extern std::size_t write(socket_t fd, const void* buf, size_t count);
    extern std::size_t read(socket_t fd, const void* buf, size_t count);

    extern std::size_t getBytesReadableOnSocket(socket_t fd);

} // namespace socket
} // namespace hare

#endif // !_HARE_NET_SOCKET_OP_H_