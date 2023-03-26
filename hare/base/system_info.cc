#include "hare/base/util/file.h"
#include <hare/base/util/system_info.h>
#include <hare/base/exception.h>
#include <hare/base/system_check.h>

#include <array>

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

        class SystemInfo {
            int32_t pid_ { 0 };
            std::array<char, NAME_LENGTH> host_name_ {};
            std::array<char, NAME_LENGTH> system_dir_ {};

        public:
            SystemInfo()
            {
#ifdef H_OS_WIN32
#elif defined(H_OS_LINUX)

                // Get the process id
                pid_ = ::getpid();

                // Get the host name
                if (::gethostname(host_name_.begin(), sizeof(host_name_)) == 0) {
                    host_name_[sizeof(host_name_) - 1] = '\0';
                }

                // Get application dir from "/proc/self/exe"
                auto size = ::readlink("/proc/self/exe", system_dir_.begin(), NAME_LENGTH);
                if (size == -1) {
                    throw Exception("Cannot read exe[/proc/self/exe] file.");
                }

                auto npos = size - 1;
                for (; npos >= 0; --npos) {
                    if (system_dir_[npos] == '/') {
                        break;
                    }
                }
                system_dir_[npos] = '\0';
#endif
            }

            inline auto pid() const -> int32_t { return pid_; }
            inline auto system_dir() const -> std::string { return system_dir_.begin(); }
            inline auto host_name() const -> std::string { return host_name_.begin(); }
        };

        static struct SystemInfo s_system_info;

    } // namespace detail

    auto systemDir() -> std::string
    {
        return detail::s_system_info.system_dir();
    }

    auto pid() -> int32_t
    {
        return detail::s_system_info.pid();
    }

    auto hostname() -> std::string
    {
        return detail::s_system_info.host_name();
    }

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
        std::array<char, INET6_ADDRSTRLEN> address_buffer {};

        switch (type) {
        case AF_INET6:
            adress_buf_len = INET6_ADDRSTRLEN;
            break;
        case AF_INET:
            adress_buf_len = INET_ADDRSTRLEN;
            break;
        default:
            throw Exception("Unknown type of ip.");
        }

        while (if_addrs != nullptr) {
            if (type == if_addrs->ifa_addr->sa_family) {
                // Is a valid IPv4 Address
                auto* tmp = &(reinterpret_cast<struct sockaddr_in*>(if_addrs->ifa_addr))->sin_addr;
                ::inet_ntop(type, tmp, address_buffer.begin(), adress_buf_len);
                ip_list.emplace_back(address_buffer.begin(), adress_buf_len);
                address_buffer.fill(0);
            }
            if_addrs = if_addrs->ifa_next;
        }
        return ret;
    }

} // namespace util
} // namespace hare