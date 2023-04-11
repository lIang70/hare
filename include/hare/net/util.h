#ifndef _HARE_NET_UTIL_H_
#define _HARE_NET_UTIL_H_

#include <hare/base/error.h>

#include <list>

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;

namespace hare {
namespace net {

    HARE_API auto hostToNetwork64(uint64_t host64) -> uint64_t;
    HARE_API auto hostToNetwork32(uint32_t host32) -> uint32_t;
    HARE_API auto hostToNetwork16(uint16_t host16) -> uint16_t;
    HARE_API auto networkToHost64(uint64_t net64) -> uint64_t;
    HARE_API auto networkToHost32(uint32_t net32) -> uint32_t;
    HARE_API auto networkToHost16(uint16_t net16) -> uint16_t;

    HARE_API inline auto sockaddrCast(const struct sockaddr_in6* addr) -> const struct sockaddr* { return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr)); }
    HARE_API inline auto sockaddrCast(struct sockaddr_in6* addr) -> struct sockaddr* { return static_cast<struct sockaddr*>(implicit_cast<void*>(addr)); }
    HARE_API inline auto sockaddrCast(const struct sockaddr_in* addr) -> const struct sockaddr* { return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr)); }
    HARE_API inline auto sockaddrInCast(const struct sockaddr* addr) -> const struct sockaddr_in* { return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr)); }
    HARE_API inline auto sockaddrIn6Cast(const struct sockaddr* addr) -> const struct sockaddr_in6* { return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(addr)); }

    HARE_API auto getLocalIp(int32_t type, std::list<std::string>& ip_list) -> Error;

} // namespace net
} // namespace hare

#endif // !_HARE_NET_UTIL_H_