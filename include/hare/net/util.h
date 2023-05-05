#ifndef _HARE_NET_UTIL_H_
#define _HARE_NET_UTIL_H_

#include <hare/base/error.h>

#include <array>
#include <list>

#define HARE_DEFAULT_HIGH_WATER (64 * 1024 * 1024)

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;

namespace hare {
namespace net {

    inline auto sockaddr_cast(const struct sockaddr_in6* _addr) -> const struct sockaddr* { return static_cast<const struct sockaddr*>(implicit_cast<const void*>(_addr)); }
    inline auto sockaddr_cast(struct sockaddr_in6* _addr) -> struct sockaddr* { return static_cast<struct sockaddr*>(implicit_cast<void*>(_addr)); }
    inline auto sockaddr_cast(const struct sockaddr_in* _addr) -> const struct sockaddr* { return static_cast<const struct sockaddr*>(implicit_cast<const void*>(_addr)); }
    inline auto sockaddr_in_cast(const struct sockaddr* _addr) -> const struct sockaddr_in* { return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(_addr)); }
    inline auto sockaddr_in6_cast(const struct sockaddr* _addr) -> const struct sockaddr_in6* { return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(_addr)); }

    HARE_API auto host_to_network64(uint64_t _host64) -> uint64_t;
    HARE_API auto host_to_network32(uint32_t _host32) -> uint32_t;
    HARE_API auto host_to_network16(uint16_t _host16) -> uint16_t;
    HARE_API auto network_to_host64(uint64_t _net64) -> uint64_t;
    HARE_API auto network_to_host32(uint32_t _net32) -> uint32_t;
    HARE_API auto network_to_host16(uint16_t _net16) -> uint16_t;

    HARE_API auto get_local_address(int32_t _type, std::list<std::string>& _addr_list) -> error;
    HARE_API auto get_socket_pair(int8_t _family, int32_t _type, int32_t _protocol, std::array<util_socket_t, 2>& _sockets) -> error;

} // namespace net
} // namespace hare

#endif // !_HARE_NET_UTIL_H_