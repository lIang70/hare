#include <hare/base/util/system_info.h>

#include <hare/base/exception.h>
#include <hare/base/util/system_check.h>

#ifdef H_OS_WIN32
#elif defined(H_OS_LINUX)
#include <unistd.h>
#endif

#define NAME_LENGTH 256

namespace hare {
namespace util {

    namespace detail {

        class SystemInfo {
            int32_t pid_ { 0 };
            char host_name_[NAME_LENGTH] {};
            char system_dir_[NAME_LENGTH] {};

        public:
            SystemInfo()
            {
#ifdef H_OS_WIN32
#elif defined(H_OS_LINUX)

                // Get the process id
                pid_ = ::getpid();

                // Get the host name
                if (::gethostname(host_name_, sizeof(host_name_)) == 0) {
                    host_name_[sizeof(host_name_) - 1] = '\0';
                }

                // Get application dir from "/proc/self/exe"
                auto size = ::readlink("/proc/self/exe", system_dir_, NAME_LENGTH);
                if (size == -1) {
                    throw Exception("Cannot read exe[/proc/self/exe] file.");
                }

                auto npos = size - 1;
                for (; npos >= 0; --npos) {
                    if (system_dir_[npos] == '/') {
                        break;
                    }
                }
                system_dir_[npos + 1] = '\0';
#endif
            }

            inline auto pid() const -> int32_t { return pid_; }
            inline auto system_dir() const -> std::string { return system_dir_; }
            inline auto host_name() const -> std::string { return host_name_; }
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

} // namespace util
} // namespace hare