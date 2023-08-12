#include <hare/base/exception.h>
#include <hare/base/fwd-inl.h>
#include <hare/base/util/system.h>
#include <hare/base/util/system_check.h>

#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <thread>

#ifndef H_OS_WIN
#include <arpa/inet.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <Windows.h>
#include <direct.h>
#include <io.h>
#include <iphlpapi.h>
#include <sys/stat.h>
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "WS2_32.lib")

#define getcwd _getcwd
#define getpid _getpid
#define fileno _fileno

const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO {
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)
void SetThreadName(DWORD dwThreadID, const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable : 6320 6322)
    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
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

        class SystemInfo {
            std::array<char, NAME_LENGTH> host_name_ {};
            std::array<char, NAME_LENGTH> system_dir_ {};
            std::int32_t pid_ { 0 };
            std::size_t page_size_ { 0 };

        public:
            static auto instance() -> SystemInfo
            {
                static SystemInfo system_info {};
                return system_info;
            }

            SystemInfo()
            {
                // Get the host name
                if (::gethostname(host_name_.data(), NAME_LENGTH) == 0) {
                    host_name_[host_name_.size() - 1] = '\0';
                }

                // Get application dir
                if (::getcwd(system_dir_.data(), NAME_LENGTH) == nullptr) {
                    throw Exception("Cannot call ::getcwd.");
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

            friend auto util::SystemDir() -> std::string;
            friend auto util::HostName() -> std::string;
            friend auto util::Pid() -> std::int32_t;
            friend auto util::PageSize() -> std::size_t;
        };

        static auto CpuTotalOccupy() -> std::uint64_t
        {
#ifndef H_OS_WIN
            // different mode cpu occupy time
            std::uint64_t user_time {};
            std::uint64_t nice_time {};
            std::uint64_t system_time {};
            std::uint64_t idle_time {};
            std::ifstream file_stream {};
            std::string file_line {};
            std::string::size_type index {};

            file_stream.open("/proc/stat");
            if (file_stream.fail()) {
                return 0;
            }

            std::getline(file_stream, file_line, '\n');
            file_stream.close();

            index = file_line.find(' ');

            file_line = file_line.substr(index);
            while (file_line[index++] == ' ') { }
            index = file_line.find(' ', index);
            if (index == std::string::npos) {
                return 0;
            }
            user_time = std::stoull(file_line);

            file_line = file_line.substr(index);
            while (file_line[index++] == ' ') { }
            index = file_line.find(' ', index);
            if (index == std::string::npos) {
                return 0;
            }
            nice_time = std::stoull(file_line);

            file_line = file_line.substr(index);
            while (file_line[index++] == ' ') { }
            index = file_line.find(' ', index);
            if (index == std::string::npos) {
                return 0;
            }
            system_time = std::stoull(file_line);

            file_line = file_line.substr(index);
            while (file_line[index++] == ' ') { }
            index = file_line.find(' ', index);
            if (index == std::string::npos) {
                return 0;
            }
            idle_time = std::stoull(file_line);

            return (user_time + nice_time + system_time + idle_time);
#else
            /// TODO(l1ang70)
            return 0;
#endif
        }

        static auto CpuProcOccupy(std::int32_t _pid) -> std::uint64_t
        {
#ifndef H_OS_WIN
#define PROCESS_ITEM 14
            // get specific pid cpu use time
            std::uint32_t tmp_pid {};
            std::uint64_t user_time {};
            std::uint64_t system_time {};
            std::uint64_t cutime {}; // all user time
            std::uint64_t cstime {}; // all dead time
            std::ifstream file_stream {};
            std::string file_line {};
            std::string::size_type index {};

            file_stream.open(fmt::format("/proc/{}/stat", _pid));
            if (file_stream.fail()) {
                return 0;
            }

            std::getline(file_stream, file_line, '\n');
            file_stream.close();

            auto len = file_line.size();
            auto count { 0 };

            for (decltype(len) i = 0; i < len; ++i) {
                if (' ' == file_line[i]) {
                    count++;
                    if (count == PROCESS_ITEM - 1) {
                        ++i;
                        file_line = file_line.substr(i);
                        break;
                    }
                }
            }

            index = file_line.find(' ');
            if (index == std::string::npos) {
                return 0;
            }
            user_time = std::stoull(file_line);

            file_line = file_line.substr(index);
            while (file_line[index++] == ' ') { }
            index = file_line.find(' ', index);
            if (index == std::string::npos) {
                return 0;
            }
            system_time = std::stoull(file_line);

            file_line = file_line.substr(index);
            while (file_line[index++] == ' ') { }
            index = file_line.find(' ', index);
            if (index == std::string::npos) {
                return 0;
            }
            cutime = std::stoull(file_line);

            file_line = file_line.substr(index);
            while (file_line[index++] == ' ') { }
            index = file_line.find(' ', index);
            if (index == std::string::npos) {
                return 0;
            }
            cstime = std::stoull(file_line);

            return (user_time + system_time + cutime + cstime);
#undef PROCESS_ITEM
#else
            /// TODO(l1ang70)
            return 0;
#endif
        }

        static auto NProcs() -> std::int64_t
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

    auto SystemDir() -> std::string
    {
        return detail::SystemInfo::instance().system_dir_.data();
    }

    auto HostName() -> std::string
    {
        return detail::SystemInfo::instance().host_name_.data();
    }

    auto Pid() -> std::int32_t
    {
        return detail::SystemInfo::instance().pid_;
    }

    auto PageSize() -> std::size_t
    {
        return detail::SystemInfo::instance().page_size_;
    }

    auto CpuUsage(std::int32_t _pid) -> double
    {
#ifdef H_OS_WIN
        MSG_TRACE("cpu usage calculation is not supported under windows.");
        return (0.0);
#endif
        auto total_cputime1 = detail::CpuTotalOccupy();
        auto pro_cputime1 = detail::CpuProcOccupy(_pid);

        // sleep 200ms to fetch two time point cpu usage snapshots sample for later calculation.
        std::this_thread::sleep_for(std::chrono::microseconds(SLEEP_TIME_SLICE_MICROS));

        auto total_cputime2 = detail::CpuTotalOccupy();
        auto pro_cputime2 = detail::CpuProcOccupy(_pid);

        auto pcpu { 0.0 };
        if (0 != total_cputime2 - total_cputime1) {
            pcpu = static_cast<double>(pro_cputime2 - pro_cputime1) / static_cast<double>(total_cputime2 - total_cputime1); // double number
        }

        auto cpu_num = detail::NProcs();
        pcpu *= static_cast<double>(cpu_num); // should multiply cpu num in multiple cpu machine

        return pcpu;
    }

    auto StackTrace(bool _demangle) -> std::string
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

    auto SetCurrentThreadName(const char* _tname) -> bool
    {
        auto ret = -1;
#ifndef H_OS_WIN
        ret = ::prctl(PR_SET_NAME, _tname);
#else
        ::SetThreadName(::GetCurrentThreadId(), _tname);
#endif
        return ret != -1;
    }

    auto ErrnoStr(std::int32_t _errorno) -> const char*
    {
        static thread_local std::array<char, HARE_SMALL_FIXED_SIZE * HARE_SMALL_FIXED_SIZE / 2> t_errno_buf;
#ifdef H_OS_WIN32
        ::strerror_s(t_errno_buf.data(), t_errno_buf.size(), _errorno);
#else
        ::strerror_r(_errorno, t_errno_buf.data(), t_errno_buf.size());
#endif
        return t_errno_buf.data();
    }

    auto FileOpen(std::FILE** _fp, const filename_t& _filename, const filename_t& _mode) -> bool
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

    auto FileExists(const filename_t& _filepath) -> bool
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

    auto FileRemove(const filename_t& _filepath) -> bool
    {
        if (!FileExists(_filepath)) {
            return false;
        }
        // TODO(l1ang70): remove file in disk;
        return std::remove(FilenameToStr(_filepath).c_str()) == 0;
    }

    auto FileSize(std::FILE* _fp) -> std::size_t
    {
        if (_fp == nullptr) {
            return 0;
        }
        auto fd = ::fileno(_fp);
#ifndef H_OS_WIN
#ifdef __USE_LARGEFILE64
#define STAT struct stat64
#define FSTAT ::fstat64
#else
#define STAT struct stat
#define FSTAT ::fstat
#endif
        STAT st {};
        if (FSTAT(fd, &st) == 0) {
            return static_cast<std::size_t>(st.st_size);
        } else {
            return 0;
        }
#undef STAT
#undef FSTAT
#else
        auto size = ::_filelengthi64(fd);
        return size < 0 ? 0 : static_cast<std::size_t>(size);
#endif
    }

    auto FileSync(std::FILE* _fp) -> bool
    {
#if defined(H_OS_LINUX)
        return ::fsync(::fileno(_fp)) == 0;
#elif defined(H_OS_WIN32)
        return ::FlushFileBuffers((HANDLE)::_get_osfhandle(::fileno(_fp)));
#endif
    }

    auto LocalAddress(std::uint8_t _family, std::list<std::string>& _addr_list) -> std::int32_t
    {
#ifndef H_OS_WIN
        // Get the list of ip addresses of machine
        ::ifaddrs* if_addrs { nullptr };
        auto ret = ::getifaddrs(&if_addrs);

        if (ret != 0) {
            return -1;
        }

        auto adress_buf_len { 0 };
        std::array<char, INET6_ADDRSTRLEN> address_buffer {};

        switch (_family) {
        case AF_INET6:
            adress_buf_len = INET6_ADDRSTRLEN;
            break;
        case AF_INET:
            adress_buf_len = INET_ADDRSTRLEN;
            break;
        default:
            return -1;
        }

        while (if_addrs != nullptr) {
            if (_family == if_addrs->ifa_addr->sa_family) {
                // Is a valid IPv4 Address
                auto* tmp = &(reinterpret_cast<struct sockaddr_in*>(if_addrs->ifa_addr))->sin_addr;
                ::inet_ntop(_family, tmp, address_buffer.data(), adress_buf_len);
                _addr_list.emplace_back(address_buffer.data(), adress_buf_len);
                address_buffer.fill('\0');
            }
            if_addrs = if_addrs->ifa_next;
        }
        return 0;
#else
        PIP_ADAPTER_INFO ip_adapter_info {};
        ULONG size {};
        if (::GetAdaptersInfo(nullptr, &size) != ERROR_SUCCESS) {
            return -1;
        }

        ip_adapter_info = (PIP_ADAPTER_INFO)::GlobalAlloc(GPTR, size);
        if (::GetAdaptersInfo(ip_adapter_info, &size) == ERROR_SUCCESS) {
            while (ip_adapter_info != nullptr) {
                _addr_list.push_back(ip_adapter_info->IpAddressList.IpAddress.String);
                ip_adapter_info = ip_adapter_info->Next;
            }
        }
        ::GlobalFree(ip_adapter_info);

        return 0;
#endif
    }

} // namespace util
} // namespace hare
