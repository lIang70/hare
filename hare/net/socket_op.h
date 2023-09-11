#ifndef _HARE_BASE_IO_SOCKET_OP_INL_H_
#define _HARE_BASE_IO_SOCKET_OP_INL_H_

#include <hare/base/io/operation.h>

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;

namespace hare {
namespace socket_op {

HARE_INLINE auto SockaddrCast(const struct sockaddr_in6* _addr) -> const
    struct sockaddr* {
  return static_cast<const struct sockaddr*>(ImplicitCast<const void*>(_addr));
}
HARE_INLINE auto SockaddrCast(struct sockaddr_in6* _addr) -> struct sockaddr* {
  return static_cast<struct sockaddr*>(ImplicitCast<void*>(_addr));
}
HARE_INLINE auto SockaddrCast(const struct sockaddr_in* _addr) -> const
    struct sockaddr* {
  return static_cast<const struct sockaddr*>(ImplicitCast<const void*>(_addr));
}
HARE_INLINE auto SockaddrCast(struct sockaddr_in* _addr) -> struct sockaddr* {
  return static_cast<struct sockaddr*>(ImplicitCast<void*>(_addr));
}
HARE_INLINE auto SockaddrCastIn(const struct sockaddr* _addr) -> const
    struct sockaddr_in* {
  return static_cast<const struct sockaddr_in*>(
      ImplicitCast<const void*>(_addr));
}
HARE_INLINE auto SockaddrCastIn(struct sockaddr* _addr) -> struct sockaddr_in* {
  return static_cast<struct sockaddr_in*>(ImplicitCast<void*>(_addr));
}
HARE_INLINE auto SockaddrCastIn6(const struct sockaddr* _addr) -> const
    struct sockaddr_in6* {
  return static_cast<const struct sockaddr_in6*>(
      ImplicitCast<const void*>(_addr));
}
HARE_INLINE auto SockaddrCastIn6(struct sockaddr* _addr)
    -> struct sockaddr_in6* {
  return static_cast<struct sockaddr_in6*>(ImplicitCast<void*>(_addr));
}

HARE_API auto CreateNonblockingOrDie(std::uint8_t _family) -> util_socket_t;
HARE_API auto Close(util_socket_t _fd) -> std::int32_t;
HARE_API auto Write(util_socket_t _fd, const void* _buf, std::size_t _size)
    -> std::int64_t;
HARE_API auto Read(util_socket_t _fd, void* _buf, std::size_t _size)
    -> std::int64_t;

HARE_API auto Bind(util_socket_t _fd, const struct sockaddr* _addr,
                   std::size_t _addr_len) -> bool;
HARE_API auto Listen(util_socket_t _fd) -> bool;
HARE_API auto Connect(util_socket_t _fd, const struct sockaddr* _addr,
                      std::size_t _addr_len) -> bool;
HARE_API auto Accept(util_socket_t _fd, struct sockaddr* _addr,
                     std::size_t _addr_len) -> util_socket_t;

HARE_API auto AddrLen(std::uint8_t _family) -> std::size_t;
HARE_API auto GetBytesReadableOnSocket(util_socket_t _fd) -> std::size_t;

HARE_API void ToIpPort(char* _buf, std::size_t _size,
                       const struct sockaddr* _addr);
HARE_API void ToIp(char* _buf, std::size_t _size, const struct sockaddr* _addr);
HARE_API auto FromIpPort(const char* _ip, std::uint16_t _port,
                         struct sockaddr_in* _addr) -> bool;
HARE_API auto FromIpPort(const char* _ip, std::uint16_t _port,
                         struct sockaddr_in6* _addr) -> bool;

}  // namespace socket_op
}  // namespace hare

#endif  // _HARE_BASE_IO_SOCKET_OP_INL_H_