#ifndef _HARE_NET_SOCKET_OP_H_
#define _HARE_NET_SOCKET_OP_H_

#include <hare/net/util.h>
#include <hare/hare-config.h>

struct sockaddr_in;
struct sockaddr_in6;
struct sockaddr;

namespace hare {
namespace socket {

    inline uint64_t hostToNetwork64(uint64_t host64)
    {
        return htobe64(host64);
    }

    inline uint32_t hostToNetwork32(uint32_t host32)
    {
        return htobe32(host32);
    }

    inline uint16_t hostToNetwork16(uint16_t host16)
    {
        return htobe16(host16);
    }

    inline uint64_t networkToHost64(uint64_t net64)
    {
        return be64toh(net64);
    }

    inline uint32_t networkToHost32(uint32_t net32)
    {
        return be32toh(net32);
    }

    inline uint16_t networkToHost16(uint16_t net16)
    {
        return be16toh(net16);
    }

    extern const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
    extern struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);
    extern const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
    extern const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
    extern const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

    extern util_socket_t createNonblockingOrDie(int8_t family);
    extern void bindOrDie(util_socket_t fd, const struct sockaddr* addr);
    extern void listenOrDie(util_socket_t fd);

    extern int32_t close(util_socket_t fd);
    extern util_socket_t accept(util_socket_t fd, struct sockaddr_in6* addr);
    extern void shutdownWrite(util_socket_t fd);
    extern std::size_t write(util_socket_t fd, const void* buf, size_t count);
    extern std::size_t read(util_socket_t fd, void* buf, size_t count);

    extern std::size_t getBytesReadableOnSocket(util_socket_t fd);
    extern std::string getSocketErrorInfo(util_socket_t fd);

    extern void toIpPort(char* buf, size_t size, const struct sockaddr* addr);
    extern void toIp(char* buf, size_t size, const struct sockaddr* addr);
    extern void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
    extern void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);

} // namespace socket
} // namespace hare

#endif // !_HARE_NET_SOCKET_OP_H_