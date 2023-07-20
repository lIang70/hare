#include <hare/base/exception.h>
#include <hare/base/util/system.h>
#include <hare/base/util/system_check.h>

#include <array>
#include <chrono>
#include <cstring>
#include <thread>

#ifndef H_OS_WIN
#include <cxxabi.h>
#include <execinfo.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <Windows.h>
#include <direct.h>
#include <io.h>

#define getcwd _getcwd
#define getpid _getpid
#define fileno _fileno

const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO {
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
void SetThreadName(DWORD dwThreadID, const char* threadName) {
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try{
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    }
    __except (EXCEPTION_EXECUTE_HANDLER){
    }
#pragma warning(pop)
}
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
                // Get the host name
                if (::gethostname(host_name_.data(), NAME_LENGTH) == 0) {
                    host_name_[host_name_.size() - 1] = '\0';
                }
                // Get application dir
                if (::getcwd(system_dir_.data(), NAME_LENGTH) == 0) {
                    throw exception("Cannot call ::getcwd.");
                }

                pid_ = ::getpid();

#ifndef H_OS_WIN
                page_size_ = ::sysconf(_SC_PAGESIZE);
#else
                SYSTEM_INFO system_info;
                ::GetSystemInfo(&system_info);
                page_size_ = system_info.dwPageSize;
#endif
            }

            friend auto util::system_dir() -> std::string;
            friend auto util::hostname() -> std::string;
            friend auto util::pid() -> std::int32_t;
            friend auto util::page_size() -> std::size_t;
        };

        static auto get_cpu_total_occupy() -> std::uint64_t
        {
#ifndef H_OS_WIN
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
#else
            /// TODO(l1ang70)
            return 0;
#endif
        }

        static auto get_cpu_proc_occupy(std::int32_t _pid) -> std::uint64_t
        {
#ifndef H_OS_WIN
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
#else
            /// TODO(l1ang70)
            return 0;
#endif
        }

        static auto get_nprocs() -> std::int64_t
        {
#ifndef H_OS_WIN
            return ::sysconf(_SC_NPROCESSORS_ONLN);
#else
            SYSTEM_INFO system_info;
            ::GetSystemInfo(&system_info);
            return system_info.dwNumberOfProcessors;
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
    }

    auto stack_trace(bool _demangle) -> std::string
    {
        static const auto MAX_STACK_FRAMES = 20;
        std::string stack {};

#ifndef H_OS_WIN
        static const auto DEMANGLE_SIZE = 256;

        std::array<void*, MAX_STACK_FRAMES> frames {};
        auto nptrs = ::backtrace(frames.data(), MAX_STACK_FRAMES);
        auto* strings = ::backtrace_symbols(frames.data(), nptrs);
        if (strings == nullptr) {
            return stack;
        }
        std::size_t len = DEMANGLE_SIZE;
        auto* demangled = _demangle ? static_cast<char*>(::malloc(len)) : nullptr;
        for (auto i = 1; i < nptrs; ++i) {
            // skipping the 0-th, which is this function.
            if (_demangle) {
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
#else
        /// TODO(l1ang70)
        return stack;
#endif
    }

    auto set_thread_name(const char* _tname) -> bool
    {
        auto ret = -1;
#ifndef H_OS_WIN
        ret = ::prctl(PR_SET_NAME, _tname);
#else
        ::SetThreadName(::GetCurrentThreadId(), _tname);
#endif
        return ret != -1;
    }

    auto errnostr(std::int32_t _errorno) -> const char*
    {
        static thread_local std::array<char, static_cast<std::size_t>(HARE_SMALL_FIXED_SIZE * HARE_SMALL_FIXED_SIZE) / 2> t_errno_buf;
#ifdef H_OS_WIN32
        ::strerror_s(t_errno_buf.data(), t_errno_buf.size(), _errorno);
#else
        ::strerror_r(_errorno, t_errno_buf.data(), t_errno_buf.size());
#endif
        return t_errno_buf.data();
    }

    auto open_s(std::FILE** _fp, const filename_t& _filename, const filename_t& _mode) -> bool
    {
#if defined(H_OS_WIN32) && HARE_WCHAR_FILENAME
        *_fp = ::_wfsopen((_filename.c_str()), _mode.c_str(), _SH_DENYWR);
#elif defined(H_OS_WIN32)
        *_fp = ::_fsopen((_filename.c_str()), _mode.c_str(), _SH_DENYWR);
#else // unix
        *_fp = ::fopen((_filename.c_str()), _mode.c_str());
#endif
        return *_fp != nullptr;
    }

    auto fexists(const filename_t& _filepath) -> bool
    {
#if defined(H_OS_LINUX) // common linux/unix all have the stat system call
        struct stat buffer { };
        return (::stat(_filepath.c_str(), &buffer) == 0);
#elif defined(H_OS_WIN32)
        WIN32_FIND_DATA wfd {};
        auto hFind = ::FindFirstFile(_filepath.data(), &wfd);
        auto ret = hFind != INVALID_HANDLE_VALUE;
        ::CloseHandle(hFind);
        return ret;
#endif
    }
    
    auto fremove(const filename_t& _filepath) -> bool
    {
        if (!fexists(_filepath)) {
            return false;
        }

        return std::remove(filename_to_str(_filepath).c_str()) == 0;
    }

    auto fsize(std::FILE* _fp) -> std::size_t
    {
        if (_fp == nullptr) {
            return 0;
        }
        auto fd = ::fileno(_fp);
#ifdef __USE_LARGEFILE64
#define STAT struct stat64
#define FSTAT fstat64
#else
#define STAT struct stat
#define FSTAT fstat
#endif
        STAT st {};
        if (FSTAT(fd, &st) == 0) {
            return static_cast<std::size_t>(st.st_size);
        } else {
            return 0;
        }
#undef STAT
#undef FSTAT
    }

    auto fsync(std::FILE* _fp) -> bool
    {
#if defined(H_OS_LINUX)
        return ::fsync(::fileno(_fp)) == 0;
#elif defined(H_OS_WIN32)
        return ::FlushFileBuffers((HANDLE)::_get_osfhandle(::fileno(_fp)));
#endif
    }

} // namespace util
} // namespace hare
