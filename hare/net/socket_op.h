#ifndef _HARE_NET_SOCKET_OP_H_
#define _HARE_NET_SOCKET_OP_H_

#include <hare/hare-config.h>
#include <hare/net/util.h>

#include <string>

namespace hare {
namespace socket {

    extern auto createNonblockingOrDie(int8_t family) -> util_socket_t;
    extern auto createDgramOrDie(int8_t family) -> util_socket_t;
    
    extern auto bind(util_socket_t target_fd, const struct sockaddr* addr) -> Error;
    extern auto listen(util_socket_t target_fd) -> Error;
    extern auto connect(util_socket_t target_fd, const struct sockaddr* addr) -> Error;

    extern auto close(util_socket_t target_fd) -> Error;
    extern auto accept(util_socket_t target_fd, struct sockaddr_in6* addr) -> util_socket_t;
    extern auto shutdownWrite(util_socket_t target_fd) -> Error;
    extern auto write(util_socket_t target_fd, const void* buf, size_t size) -> ssize_t;
    extern auto read(util_socket_t target_fd, void* buf, size_t size) -> ssize_t;
    extern auto recvfrom(util_socket_t target_fd, void* buf, size_t size, struct sockaddr* addr, size_t addr_len) -> ssize_t;

    extern auto getBytesReadableOnSocket(util_socket_t target_fd) -> std::size_t;
    extern auto getSocketErrorInfo(util_socket_t target_fd) -> std::string;

    extern void toIpPort(char* buf, size_t size, const struct sockaddr* addr);
    extern void toIp(char* buf, size_t size, const struct sockaddr* addr);
    extern auto fromIpPort(const char* target_ip, uint16_t port, struct sockaddr_in* addr) -> Error;
    extern auto fromIpPort(const char* target_ip, uint16_t port, struct sockaddr_in6* addr) -> Error;

} // namespace socket
} // namespace hare

#endif // !_HARE_NET_SOCKET_OP_H_