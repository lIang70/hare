/**
 * @file hare/net/util.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with util.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_UTIL_H_
#define _HARE_NET_UTIL_H_

#include <hare/net/error.h>

#include <list>

#define HARE_DEFAULT_HIGH_WATER (64 * 1024 * 1024)

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;

namespace hare {
namespace net {

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

    HARE_API auto get_local_address(std::int32_t _type, std::list<std::string>& _addr_list) -> error;
    HARE_API auto get_socket_pair(std::int8_t _family, std::int32_t _type, std::int32_t _protocol, std::array<util_socket_t, 2>& _sockets) -> error;

} // namespace net
} // namespace hare

#endif // _HARE_NET_UTIL_H_