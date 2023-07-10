#include <cstddef>
#include <cstdint>
#include <hare/base/util/system_info.h>

#include <hare/base/exception.h>
#include <hare/base/util/system_check.h>

#include <array>
#include <chrono>
#include <cstring>
#include <thread>

#ifdef H_OS_LINUX
#include <cxxabi.h>
#include <execinfo.h>
#include <sys/prctl.h>
#include <unistd.h>
#endif

#define NAME_LENGTH 256
#define FILE_LENGTH 1024

/// FIXME: the 200ms is a magic number, works well.
#define SLEEP_TIME_SLICE_MICROS 200000

namespace hare {
namespace util {

    namespace detail {

        class system_info {
            std::array<char, NAME_LENGTH> host_name_ {};
            std::array<char, NAME_LENGTH> system_dir_ {};
            std::int32_t pid_ { 0 };
            std::size_t page_size_ { 0 };

        public:
            static auto instance() -> system_info
            {
                static system_info s_info {};
                return s_info;
            }

            system_info()
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

                pid_ = ::getpid();

                page_size_ = ::sysconf(_SC_PAGESIZE);
#endif
            }

            friend auto util::system_dir() -> std::string;
            friend auto util::hostname() -> std::string;
            friend auto util::pid() -> std::int32_t;
            friend auto util::page_size() -> std::size_t;
        };

        static auto get_cpu_total_occupy() -> std::uint64_t
        {
#ifdef H_OS_LINUX
            // different mode cpu occupy time
            std::uint64_t user_time {};
            std::uint64_t nice_time {};
            std::uint64_t system_time {};
            std::uint64_t idle_time {};
            std::array<char, static_cast<std::size_t>(HARE_SMALL_FIXED_SIZE) * 2> name {};
            std::array<char, FILE_LENGTH> buffer {};

            auto* stat_fd = ::fopen("/proc/stat", "r");
            if (stat_fd == nullptr) {
                return 0;
            }

            auto* ret_c = ::fgets(buffer.data(), FILE_LENGTH, stat_fd);
            auto ret = ::sscanf(buffer.data(), "%s %ld %ld %ld %ld", name.data(), &user_time, &nice_time, &system_time, &idle_time);
            ret = ::fclose(stat_fd);

            return (user_time + nice_time + system_time + idle_time);
#endif
            return 0;
        }

        static auto get_cpu_proc_occupy(std::int32_t _pid) -> std::uint64_t
        {
#ifdef H_OS_LINUX
            static const auto PROCESS_ITEM = 14;
            // get specific pid cpu use time
            std::uint32_t tmp_pid {};
            std::uint64_t user_time {};
            std::uint64_t system_time {};
            std::uint64_t cutime {}; // all user time
            std::uint64_t cstime {}; // all dead time
            std::array<char, static_cast<std::size_t>(HARE_SMALL_FIXED_SIZE) * 2> name {};
            std::array<char, FILE_LENGTH> buffer {};

            auto ret = ::sprintf(name.data(), "/proc/%d/stat", _pid);
            auto* stat_fd = ::fopen(name.data(), "r");
            if (stat_fd == nullptr) {
                return 0;
            }

            auto* ret_c = ::fgets(buffer.data(), FILE_LENGTH, stat_fd);
            ret = ::sscanf(buffer.data(), "%u", &tmp_pid);
            auto* item = buffer.data();
            {
                auto len = ::strlen(item);
                auto count { 0 };

                for (auto i = 0; i < len; i++) {
                    if (' ' == *item) {
                        count++;
                        if (count == PROCESS_ITEM - 1) {
                            item++;
                            break;
                        }
                    }
                    item++;
                }
            }

            ret = ::sscanf(item, "%ld %ld %ld %ld", &user_time, &system_time, &cutime, &cstime);
            ret = ::fclose(stat_fd);

            return (user_time + system_time + cutime + cstime);
#undef PROCESS_ITEM
#endif
            return 0;
        }

        static auto get_nprocs() -> int64_t
        {
#ifdef H_OS_LINUX
            return ::sysconf(_SC_NPROCESSORS_ONLN);
#endif
        }
    } // namespace detail

    auto system_dir() -> std::string
    {
        return detail::system_info::instance().system_dir_.data();
    }

    auto hostname() -> std::string
    {
        return detail::system_info::instance().host_name_.data();
    }

    auto pid() -> std::int32_t
    {
        return detail::system_info::instance().pid_;
    }

    auto page_size() -> std::size_t
    {
        return detail::system_info::instance().page_size_;
    }

    auto cpu_usage(std::int32_t _pid) -> double
    {
#ifdef H_OS_LINUX
        auto total_cputime1 = detail::get_cpu_total_occupy();
        auto pro_cputime1 = detail::get_cpu_proc_occupy(_pid);

        // sleep 200ms to fetch two time point cpu usage snapshots sample for later calculation.
        std::this_thread::sleep_for(std::chrono::microseconds(SLEEP_TIME_SLICE_MICROS));

        auto total_cputime2 = detail::get_cpu_total_occupy();
        auto pro_cputime2 = detail::get_cpu_proc_occupy(_pid);

        auto pcpu { 0.0 };
        if (0 != total_cputime2 - total_cputime1) {
            pcpu = static_cast<double>(pro_cputime2 - pro_cputime1) / static_cast<double>(total_cputime2 - total_cputime1); // double number
        }

        auto cpu_num = detail::get_nprocs();
        pcpu *= static_cast<double>(cpu_num); // should multiply cpu num in multiple cpu machine

        return pcpu;
#endif
    }

    auto stack_trace(bool demangle) -> std::string
    {
        static const auto MAX_STACK_FRAMES = 20;
#ifdef H_OS_LINUX
        static const auto DEMANGLE_SIZE = 256;

        std::string stack;
        std::array<void*, MAX_STACK_FRAMES> frames {};
        auto nptrs = ::backtrace(frames.data(), MAX_STACK_FRAMES);
        auto* strings = ::backtrace_symbols(frames.data(), nptrs);
        if (strings == nullptr) {
            return stack;
        }
        std::size_t len = DEMANGLE_SIZE;
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
        ::free(demangled);
        ::free(strings);
        return stack;
#endif
    }

    auto set_thread_name(const char* tname) -> error
    {
#ifdef H_OS_LINUX
        auto ret = ::prctl(PR_SET_NAME, tname);
        if (ret == -1) {
            return error(HARE_ERROR_SET_THREAD_NAME);
        }
#endif
        return error(HARE_ERROR_SUCCESS);
    }

    auto errnostr(int _errorno) -> const char*
    {
        static thread_local std::array<char, static_cast<std::size_t>(HARE_SMALL_FIXED_SIZE* HARE_SMALL_FIXED_SIZE) / 2> t_errno_buf;
        ::strerror_r(_errorno, t_errno_buf.data(), t_errno_buf.size());
        return t_errno_buf.data();
    }

} // namespace util
} // namespace hare