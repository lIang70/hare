#include "hare/base/util/file.h"
#include <hare/base/detail/system_info.h>
#include <hare/base/exception.h>
#include <hare/base/system_check.h>

#ifdef H_OS_WIN32
#elif defined(H_OS_LINUX)
#include <unistd.h>
#endif

namespace hare {
namespace util {

    namespace detail {

        static const int32_t NAME_LENGTH = 256;

        struct SystemInfo {
            int32_t pid { 0 };
            char host_name[NAME_LENGTH] { "UNLNOWN" };
            char system_dir[NAME_LENGTH] { "NULL" };

            SystemInfo()
            {
#ifdef H_OS_WIN32
#elif defined(H_OS_LINUX)

                pid = ::getpid();

                if (::gethostname(host_name, sizeof(host_name)) == 0) {
                    host_name[sizeof(host_name) - 1] = '\0';
                }

                // /proc/self/exe
                auto size = readlink("/proc/self/exe", system_dir, NAME_LENGTH);
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

    std::string systemdir()
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

} // namespace util
} // namespace hare