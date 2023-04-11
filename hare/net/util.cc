#include "hare/base/error.h"
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

    auto hostToNetwork64(uint64_t host64) -> uint64_t { return htobe64(host64); }
    auto hostToNetwork32(uint32_t host32) -> uint32_t { return htobe32(host32); }
    auto hostToNetwork16(uint16_t host16) -> uint16_t { return htobe16(host16); }
    auto networkToHost64(uint64_t net64) -> uint64_t { return be64toh(net64); }
    auto networkToHost32(uint32_t net32) -> uint32_t { return be32toh(net32); }
    auto networkToHost16(uint16_t net16) -> uint16_t { return be16toh(net16); }

    auto getLocalIp(int32_t type, std::list<std::string>& ip_list) -> Error
    {
        // Get the list of ip addresses of machine
        ::ifaddrs* if_addrs { nullptr };
        auto ret = ::getifaddrs(&if_addrs);

        if (ret != 0) {
            return Error(HARE_ERROR_GET_LOCAL_ADDRESS);
        }

        int32_t adress_buf_len {};
        char address_buffer[INET6_ADDRSTRLEN] {};

        switch (type) {
        case AF_INET6:
            adress_buf_len = INET6_ADDRSTRLEN;
            break;
        case AF_INET:
            adress_buf_len = INET_ADDRSTRLEN;
            break;
        default:
            return Error(HARE_ERROR_WRONG_FAMILY);
        }

        while (if_addrs != nullptr) {
            if (type == if_addrs->ifa_addr->sa_family) {
                // Is a valid IPv4 Address
                auto* tmp = &(reinterpret_cast<struct sockaddr_in*>(if_addrs->ifa_addr))->sin_addr;
                ::inet_ntop(type, tmp, address_buffer, adress_buf_len);
                ip_list.emplace_back(address_buffer, adress_buf_len);
                hare::setZero(address_buffer, INET6_ADDRSTRLEN);
            }
            if_addrs = if_addrs->ifa_next;
        }
        return Error(HARE_ERROR_SUCCESS);
    }

} // namespace net
} // namespace hare