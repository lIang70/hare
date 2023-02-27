#include "hare/base/util/file.h"
#include <hare/base/detail/system_info.h>
#include <hare/base/exception.h>
#include <hare/base/system_check.h>

#ifdef H_OS_WIN32
#elif defined(H_OS_LINUX)
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#define NAME_LENGTH 256

namespace hare {
namespace util {

    namespace detail {

        struct SystemInfo {
            int32_t pid { 0 };
            char host_name[NAME_LENGTH] {};
            char system_dir[NAME_LENGTH] {};

            SystemInfo()
            {
#ifdef H_OS_WIN32
#elif defined(H_OS_LINUX)

                // Get the process id
                pid = ::getpid();

                // Get the host name
                if (::gethostname(host_name, sizeof(host_name)) == 0) {
                    host_name[sizeof(host_name) - 1] = '\0';
                }

                // Get application dir from "/proc/self/exe"
                auto size = ::readlink("/proc/self/exe", system_dir, NAME_LENGTH);
                if (size == -1) {
                    throw Exception("Cannot read exe[/proc/self/exe] file.");
                } else {
                    for (int32_t i = size - 1; i >= 0; --i) {
                        if (system_dir[i] == '/')
                            break;
                        else
                            system_dir[i] = '\0';
                    }
                }
#endif
            }
        };

        static struct SystemInfo s_system_info;

    } // namespace detail

    std::string systemDir()
    {
        return detail::s_system_info.system_dir;
    }

    int32_t pid()
    {
        return detail::s_system_info.pid;
    }

    std::string hostname()
    {
        return detail::s_system_info.host_name;
    }

    int32_t getLocalIp(int32_t type, std::list<std::string>& ip_list)
    {
        // Get the list of ip addresses of machine
        ::ifaddrs* if_addrs { nullptr };
        auto ret = ::getifaddrs(&if_addrs);

        if (ret) {
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
            throw Exception("Unknown type of ip.");
            break;
        }

        while (if_addrs) {
            if (type == if_addrs->ifa_addr->sa_family) {
                // Is a valid IPv4 Address
                auto tmp = &((struct sockaddr_in*)if_addrs->ifa_addr)->sin_addr;
                ::inet_ntop(type, tmp, address_buffer, adress_buf_len);
                ip_list.emplace_back(address_buffer, adress_buf_len);
                memset(address_buffer, 0, adress_buf_len);
            }
            if_addrs = if_addrs->ifa_next;
        }
        return ret;
    }

} // namespace util
} // namespace hare