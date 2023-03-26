#include "hare/base/util/util.h"
#include <cstring>
#include <hare/net/util.h>
#include <hare/base/system_check.h>

#include <cerrno>
#include <string>

#ifdef H_OS_WIN32
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#endif

namespace hare {
namespace net {

    auto getLocalIp(int32_t type, std::list<std::string>& ip_list) -> int32_t
    {
        // Get the list of ip addresses of machine
        ::ifaddrs* if_addrs { nullptr };
        auto ret = ::getifaddrs(&if_addrs);

        if (ret != 0) {
            ret = errno;
            return ret;
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
            return (-1);
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
        return ret;
    }

} // namespace net
} // namespace hare