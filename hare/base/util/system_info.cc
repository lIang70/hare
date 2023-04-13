#include <hare/base/util/system_info.h>

#include <hare/base/exception.h>
#include <hare/base/util/system_check.h>

#include <array>

#ifdef H_OS_LINUX
#include <cxxabi.h>
#include <execinfo.h>
#include <sys/prctl.h>
#include <unistd.h>
#endif

#define NAME_LENGTH 256

namespace hare {
namespace util {

    namespace detail {

        class SystemInfo {
            std::array<char, NAME_LENGTH> host_name_ {};
            std::array<char, NAME_LENGTH> system_dir_ {};

        public:
            SystemInfo()
            {
#ifdef H_OS_LINUX

                // Get the host name
                if (::gethostname(host_name_.data(), host_name_.size()) == 0) {
                    host_name_[host_name_.size() - 1] = '\0';
                }

                // Get application dir from "/proc/self/exe"
                auto size = ::readlink("/proc/self/exe", system_dir_.data(), NAME_LENGTH);
                if (size == -1) {
                    throw exception("Cannot read exe[/proc/self/exe] file.");
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

            friend auto util::system_dir() -> std::string;
            friend auto util::hostname() -> std::string;
        };

        static struct SystemInfo s_system_info;

    } // namespace detail

    auto system_dir() -> std::string
    {
        return detail::s_system_info.system_dir_.data();
    }

    auto pid() -> int32_t
    {
        return ::getpid();
    }

    auto hostname() -> std::string
    {
        return detail::s_system_info.host_name_.data();
    }

    auto stack_trace(bool demangle) -> std::string
    {
        static const int MAX_STACK_FRAMES = 20;
#ifdef H_OS_LINUX
#define DEMANGLE_SIZE 256

        std::string stack;
        std::array<void*, MAX_STACK_FRAMES> frames {};
        auto nptrs = ::backtrace(frames.data(), MAX_STACK_FRAMES);
        auto* strings = ::backtrace_symbols(frames.data(), nptrs);
        if (strings == nullptr) {
            return stack;
        }
        size_t len = DEMANGLE_SIZE;
        auto* demangled = demangle ? static_cast<char*>(::malloc(len)) : nullptr;
        for (auto i = 1; i < nptrs; ++i) {
            // skipping the 0-th, which is this function.
            if (demangle) {
                // https://panthema.net/2008/0901-stacktrace-demangled/
                // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
                char* left_par = nullptr;
                char* plus = nullptr;
                for (auto* ch = strings[i]; *ch != 0; ++ch) {
                    if (*ch == '(') {
                        left_par = ch;
                    } else if (*ch == '+') {
                        plus = ch;
                    }
                }

                if ((left_par != nullptr) && (plus != nullptr)) {
                    *plus = '\0';
                    auto status = 0;
                    auto* ret = abi::__cxa_demangle(left_par + 1, demangled, &len, &status);
                    *plus = '+';
                    if (status == 0) {
                        demangled = ret; // ret could be realloc()
                        stack.append(strings[i], left_par + 1);
                        stack.append(demangled);
                        stack.append(plus);
                        stack.push_back('\n');
                        continue;
                    }
                }
            }
            // Fallback to mangled names
            stack.append(strings[i]);
            stack.push_back('\n');
        }
        free(demangled);
        free(strings);
        return stack;
#endif
    }

    auto set_tname(const char* tname) -> error
    {
#ifdef H_OS_LINUX
        auto ret = ::prctl(PR_SET_NAME, tname);
        if (ret == -1) {
            return error(HARE_ERROR_SET_THREAD_NAME);
        }
#endif
        return error(HARE_ERROR_SUCCESS);
    }

} // namespace util
} // namespace hare