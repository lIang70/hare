#ifndef _HARE_BASE_IO_SOCKET_OP_INL_H_
#define _HARE_BASE_IO_SOCKET_OP_INL_H_

#include <hare/base/io/socket_op.h>

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;

namespace hare {
namespace socket_op {
    
    HARE_INLINE auto sockaddr_cast(const struct sockaddr_in6* _addr) -> const struct sockaddr* { return static_cast<const struct sockaddr*>(implicit_cast<const void*>(_addr)); }
    HARE_INLINE auto sockaddr_cast(struct sockaddr_in6* _addr) -> struct sockaddr* { return static_cast<struct sockaddr*>(implicit_cast<void*>(_addr)); }
    HARE_INLINE auto sockaddr_cast(const struct sockaddr_in* _addr) -> const struct sockaddr* { return static_cast<const struct sockaddr*>(implicit_cast<const void*>(_addr)); }
    HARE_INLINE auto sockaddr_cast(struct sockaddr_in* _addr) -> struct sockaddr* { return static_cast<struct sockaddr*>(implicit_cast<void*>(_addr)); }
    HARE_INLINE auto sockaddr_in_cast(const struct sockaddr* _addr) -> const struct sockaddr_in* { return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(_addr)); }
    HARE_INLINE auto sockaddr_in_cast(struct sockaddr* _addr) -> struct sockaddr_in* { return static_cast<struct sockaddr_in*>(implicit_cast<void*>(_addr)); }
    HARE_INLINE auto sockaddr_in6_cast(const struct sockaddr* _addr) -> const struct sockaddr_in6* { return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(_addr)); }
    HARE_INLINE auto sockaddr_in6_cast(struct sockaddr* _addr) -> struct sockaddr_in6* { return static_cast<struct sockaddr_in6*>(implicit_cast<void*>(_addr)); }

    HARE_API auto create_nonblocking_or_die(std::uint8_t _family) -> util_socket_t;
    HARE_API auto close(util_socket_t _fd) -> std::int32_t;
    HARE_API auto write(util_socket_t _fd, const void* _buf, std::size_t _size) -> std::int64_t;
    HARE_API auto read(util_socket_t _fd, void* _buf, std::size_t _size) -> std::int64_t;

    HARE_API auto bind(util_socket_t _fd, const struct sockaddr* _addr, std::size_t _addr_len) -> bool;
    HARE_API auto listen(util_socket_t _fd) -> bool;
    HARE_API auto connect(util_socket_t _fd, const struct sockaddr* _addr, std::size_t _addr_len) -> bool;
    HARE_API auto accept(util_socket_t _fd, struct sockaddr* _addr, std::size_t _addr_len) -> util_socket_t;

    HARE_API auto get_addr_len(std::uint8_t _family) -> std::size_t;
    HARE_API auto get_bytes_readable_on_socket(util_socket_t _fd) -> std::size_t;

    HARE_API void to_ip_port(char* _buf, std::size_t _size, const struct sockaddr* _addr);
    HARE_API void to_ip(char* _buf, std::size_t _size, const struct sockaddr* _addr);
    HARE_API auto from_ip_port(const char* _ip, std::uint16_t _port, struct sockaddr_in* _addr) -> bool;
    HARE_API auto from_ip_port(const char* _ip, std::uint16_t _port, struct sockaddr_in6* _addr) -> bool;

} // namespace socket_op
} // namespace hare

#endif // _HARE_BASE_IO_SOCKET_OP_INL_H_