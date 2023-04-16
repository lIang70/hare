#include <hare/net/util.h>

#include <cerrno>
#include <endian.h>
#include <string>

#ifdef H_OS_WIN32
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#endif

namespace hare {
namespace net {

    auto host_to_network64(uint64_t host64) -> uint64_t { return htobe64(host64); }
    auto host_to_network32(uint32_t host32) -> uint32_t { return htobe32(host32); }
    auto host_to_network16(uint16_t host16) -> uint16_t { return htobe16(host16); }
    auto network_to_host64(uint64_t net64) -> uint64_t { return be64toh(net64); }
    auto network_to_host32(uint32_t net32) -> uint32_t { return be32toh(net32); }
    auto network_to_host16(uint16_t net16) -> uint16_t { return be16toh(net16); }

    auto get_local_address(int32_t _type, std::list<std::string>& _addr_list) -> error
    {
        // Get the list of ip addresses of machine
        ::ifaddrs* if_addrs { nullptr };
        auto ret = ::getifaddrs(&if_addrs);

        if (ret != 0) {
            return error(HARE_ERROR_GET_LOCAL_ADDRESS);
        }

        int32_t adress_buf_len {};
        std::array<char, INET6_ADDRSTRLEN> address_buffer {};

        switch (_type) {
        case AF_INET6:
            adress_buf_len = INET6_ADDRSTRLEN;
            break;
        case AF_INET:
            adress_buf_len = INET_ADDRSTRLEN;
            break;
        default:
            return error(HARE_ERROR_WRONG_FAMILY);
        }

        while (if_addrs != nullptr) {
            if (_type == if_addrs->ifa_addr->sa_family) {
                // Is a valid IPv4 Address
                auto* tmp = &(reinterpret_cast<struct sockaddr_in*>(if_addrs->ifa_addr))->sin_addr;
                ::inet_ntop(_type, tmp, address_buffer.data(), adress_buf_len);
                _addr_list.emplace_back(address_buffer.data(), adress_buf_len);
                address_buffer.fill('\0');
            }
            if_addrs = if_addrs->ifa_next;
        }
        return error(HARE_ERROR_SUCCESS);
    }

    auto get_socket_pair(int8_t _family, int32_t _type, int32_t _protocol, std::array<util_socket_t, 2>& _sockets) -> error
    {
#ifndef M_OS_WIN32
        auto ret = ::socketpair(_family, _type, _protocol, _sockets.data());
        return ret < 0 ? error(HARE_ERROR_GET_SOCKET_PAIR) : error(HARE_ERROR_SUCCESS);
#endif
    }

} // namespace net
} // namespace hare