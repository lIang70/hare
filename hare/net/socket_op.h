#ifndef _HARE_NET_SOCKET_OP_H_
#define _HARE_NET_SOCKET_OP_H_

#include <hare/net/util.h>
#include <hare/hare-config.h>

#include <cinttypes>
#include <string>

struct sockaddr_in;
struct sockaddr_in6;
struct sockaddr;

namespace hare {
namespace socket {

    inline auto hostToNetwork64(uint64_t host64) -> uint64_t
    {
        return htobe64(host64);
    }

    inline auto hostToNetwork32(uint32_t host32) -> uint32_t
    {
        return htobe32(host32);
    }

    inline auto hostToNetwork16(uint16_t host16) -> uint16_t
    {
        return htobe16(host16);
    }

    inline auto networkToHost64(uint64_t net64) -> uint64_t
    {
        return be64toh(net64);
    }

    inline auto networkToHost32(uint32_t net32) -> uint32_t
    {
        return be32toh(net32);
    }

    inline auto networkToHost16(uint16_t net16) -> uint16_t
    {
        return be16toh(net16);
    }

    extern auto sockaddr_cast(const struct sockaddr_in6* addr) -> const struct sockaddr*;
    extern auto sockaddr_cast(struct sockaddr_in6* addr) -> struct sockaddr*;
    extern auto sockaddr_cast(const struct sockaddr_in* addr) -> const struct sockaddr*;
    extern auto sockaddr_in_cast(const struct sockaddr* addr) -> const struct sockaddr_in*;
    extern auto sockaddr_in6_cast(const struct sockaddr* addr) -> const struct sockaddr_in6*;

    extern auto createNonblockingOrDie(int8_t family) -> util_socket_t;
    extern auto bind(util_socket_t target_fd, const struct sockaddr* addr) -> bool;
    extern auto listen(util_socket_t target_fd) -> bool;

    extern auto close(util_socket_t target_fd) -> int32_t;
    extern auto accept(util_socket_t target_fd, struct sockaddr_in6* addr) -> util_socket_t;
    extern void shutdownWrite(util_socket_t target_fd);
    extern auto write(util_socket_t target_fd, const void* buf, size_t size) -> std::size_t;
    extern auto read(util_socket_t target_fd, void* buf, size_t size) -> std::size_t;

    extern auto getBytesReadableOnSocket(util_socket_t target_fd) -> std::size_t;
    extern auto getSocketErrorInfo(util_socket_t target_fd) -> std::string;

    extern void toIpPort(char* buf, size_t size, const struct sockaddr* addr);
    extern void toIp(char* buf, size_t size, const struct sockaddr* addr);
    extern void fromIpPort(const char* target_ip, uint16_t port, struct sockaddr_in* addr);
    extern void fromIpPort(const char* target_ip, uint16_t port, struct sockaddr_in6* addr);

} // namespace socket
} // namespace hare

#endif // !_HARE_NET_SOCKET_OP_H_