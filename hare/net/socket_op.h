#ifndef _HARE_NET_SOCKET_OP_H_
#define _HARE_NET_SOCKET_OP_H_

#include <hare/hare-config.h>
#include <hare/net/error.h>
#include <hare/net/util.h>

#include <cstddef>

namespace hare {
namespace net {
    namespace socket_op {

        extern auto create_nonblocking_or_die(std::uint8_t _family) -> util_socket_t;
        extern auto create_dgram_or_die(std::uint8_t _family) -> util_socket_t;

        extern auto bind(util_socket_t _fd, const struct sockaddr* _addr) -> error;
        extern auto listen(util_socket_t _fd) -> error;
        extern auto connect(util_socket_t _fd, const struct sockaddr* _addr) -> error;

        extern auto close(util_socket_t _fd) -> error;
        extern auto accept(util_socket_t _fd, struct sockaddr_in6* _addr) -> util_socket_t;
        extern auto shutdown_write(util_socket_t _fd) -> error;
        extern auto write(util_socket_t _fd, const void* _buf, std::size_t _size) -> std::int64_t;
        extern auto read(util_socket_t _fd, void* _buf, std::size_t _size) -> std::int64_t;
        extern auto recvfrom(util_socket_t _fd, void* _buf, std::size_t _size, struct sockaddr* _addr, std::size_t _addr_len) -> std::int64_t;
        extern auto sendto(util_socket_t _fd, void* _buf, std::size_t _size, struct sockaddr* _addr, std::size_t _addr_len) -> std::int64_t;

        extern auto get_addr_len(std::uint8_t _family) -> std::size_t;
        extern auto get_bytes_readable_on_socket(util_socket_t _fd) -> std::int64_t;
        extern auto get_socket_error_info(util_socket_t _fd) -> std::string;

        extern void to_ip_port(char* _buf, std::size_t _size, const struct sockaddr* _addr);
        extern void to_ip(char* _buf, std::size_t _size, const struct sockaddr* _addr);
        extern auto from_ip_port(const char* _ip, std::uint16_t _port, struct sockaddr_in* _addr) -> error;
        extern auto from_ip_port(const char* _ip, std::uint16_t _port, struct sockaddr_in6* _addr) -> error;

    } // namespace socket_op
} // namespace net
} // namespace hare

#endif // _HARE_NET_SOCKET_OP_H_