/**
 * @file hare/base/io/socket_op.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with socket_op.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_IO_SOCKET_OP_H_
#define _HARE_BASE_IO_SOCKET_OP_H_

#include <hare/base/fwd.h>

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;

namespace hare {
namespace socket_op {

    HARE_INLINE auto sockaddr_cast(const struct sockaddr_in6* _addr) -> const struct sockaddr* { return static_cast<const struct sockaddr*>(implicit_cast<const void*>(_addr)); }
    HARE_INLINE auto sockaddr_cast(struct sockaddr_in6* _addr) -> struct sockaddr* { return static_cast<struct sockaddr*>(implicit_cast<void*>(_addr)); }
    HARE_INLINE auto sockaddr_cast(const struct sockaddr_in* _addr) -> const struct sockaddr* { return static_cast<const struct sockaddr*>(implicit_cast<const void*>(_addr)); }
    HARE_INLINE auto sockaddr_in_cast(const struct sockaddr* _addr) -> const struct sockaddr_in* { return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(_addr)); }
    HARE_INLINE auto sockaddr_in6_cast(const struct sockaddr* _addr) -> const struct sockaddr_in6* { return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(_addr)); }

    HARE_API auto host_to_network64(std::uint64_t _host64) -> std::uint64_t;
    HARE_API auto host_to_network32(std::uint32_t _host32) -> std::uint32_t;
    HARE_API auto host_to_network16(std::uint16_t _host16) -> std::uint16_t;
    HARE_API auto network_to_host64(std::uint64_t _net64) -> std::uint64_t;
    HARE_API auto network_to_host32(std::uint32_t _net32) -> std::uint32_t;
    HARE_API auto network_to_host16(std::uint16_t _net16) -> std::uint16_t;

    HARE_API auto create_nonblocking_or_die(std::uint8_t _family) -> util_socket_t;
    HARE_API auto create_dgram_or_die(std::uint8_t _family) -> util_socket_t;

    HARE_API auto close(util_socket_t _fd) -> std::int32_t;
    HARE_API auto write(util_socket_t _fd, const void* _buf, std::size_t _size) -> std::int64_t;
    HARE_API auto read(util_socket_t _fd, void* _buf, std::size_t _size) -> std::int64_t;
    HARE_API auto sendto(util_socket_t _fd, void* _buf, std::size_t _size, struct sockaddr* _addr, std::size_t _addr_len) -> std::int64_t;
    HARE_API auto recvfrom(util_socket_t _fd, void* _buf, std::size_t _size, struct sockaddr* _addr, std::size_t _addr_len) -> std::int64_t;

    HARE_API auto bind(util_socket_t _fd, const struct sockaddr* _addr, std::size_t _addr_len) -> bool;
    HARE_API auto listen(util_socket_t _fd) -> bool;
    HARE_API auto connect(util_socket_t _fd, const struct sockaddr* _addr, std::size_t _addr_len) -> bool;
    HARE_API auto accept(util_socket_t _fd, struct sockaddr* _addr, std::size_t _addr_len) -> util_socket_t;

    HARE_API auto get_addr_len(std::uint8_t _family) -> std::size_t;
    HARE_API auto get_bytes_readable_on_socket(util_socket_t _fd) -> std::int64_t;
    HARE_API auto get_socket_error_info(util_socket_t _fd) -> std::string;

    HARE_API void to_ip_port(char* _buf, std::size_t _size, const struct sockaddr* _addr);
    HARE_API void to_ip(char* _buf, std::size_t _size, const struct sockaddr* _addr);
    HARE_API auto from_ip_port(const char* _ip, std::uint16_t _port, struct sockaddr_in* _addr) -> bool;
    HARE_API auto from_ip_port(const char* _ip, std::uint16_t _port, struct sockaddr_in6* _addr) -> bool;

    HARE_API auto socketpair(std::uint8_t _family, std::int32_t _type, std::int32_t _protocol, util_socket_t _socket[2]) -> std::int32_t;

} // namespace socket_op
} // namespace hare

#endif // _HARE_BASE_IO_SOCKET_OP_H_