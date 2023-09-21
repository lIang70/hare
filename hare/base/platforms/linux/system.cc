#include <hare/base/exception.h>
#include <hare/base/system.h>
#include <hare/base/util/system_check.h>

#include <fstream>
#include <thread>

#include "base/fwd_inl.h"

#ifdef H_OS_LINUX
#include <arpa/inet.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>

#define NAME_LENGTH 256
#define FILE_LENGTH 1024

/// FIXME: the 200ms is a magic number, works well.
#define SLEEP_TIME_SLICE_MICROS 200000

namespace hare {

namespace system_inner {

class SystemInfo {
  std::array<char, NAME_LENGTH> host_name_{};
  std::array<char, NAME_LENGTH> system_dir_{};
  std::int32_t pid_{0};
  std::size_t page_size_{0};

 public:
  static auto instance() -> SystemInfo {
    static SystemInfo system_info{};
    return system_info;
  }

 private:
  SystemInfo() {
    // Get the host name
    if (::gethostname(host_name_.data(), NAME_LENGTH) == 0) {
      host_name_[host_name_.size() - 1] = '\0';
    }

    // Get application dir
    if (::getcwd(system_dir_.data(), NAME_LENGTH) == nullptr) {
      HARE_INTERNAL_FATAL("cannot call ::getcwd.");
    }

    pid_ = ::getpid();

    page_size_ = ::sysconf(_SC_PAGESIZE);
  }

  friend auto ::hare::SystemDir() -> std::string;
  friend auto ::hare::HostName() -> std::string;
  friend auto ::hare::Pid() -> std::int32_t;
  friend auto ::hare::PageSize() -> std::size_t;
};

static auto CpuTotalOccupy() -> std::uint64_t {
  // different mode cpu occupy time
  std::uint64_t user_time{};
  std::uint64_t nice_time{};
  std::uint64_t system_time{};
  std::uint64_t idle_time{};
  std::ifstream file_stream{};
  std::string file_line{};
  std::string::size_type index{};

  file_stream.open("/proc/stat");
  if (file_stream.fail()) {
    return 0;
  }

  std::getline(file_stream, file_line, '\n');
  file_stream.close();

  index = file_line.find(' ');

  file_line = file_line.substr(index);
  while (file_line[index++] == ' ') {
  }
  index = file_line.find(' ', index);
  if (index == std::string::npos) {
    return 0;
  }
  user_time = std::stoull(file_line);

  file_line = file_line.substr(index);
  while (file_line[index++] == ' ') {
  }
  index = file_line.find(' ', index);
  if (index == std::string::npos) {
    return 0;
  }
  nice_time = std::stoull(file_line);

  file_line = file_line.substr(index);
  while (file_line[index++] == ' ') {
  }
  index = file_line.find(' ', index);
  if (index == std::string::npos) {
    return 0;
  }
  system_time = std::stoull(file_line);

  file_line = file_line.substr(index);
  while (file_line[index++] == ' ') {
  }
  index = file_line.find(' ', index);
  if (index == std::string::npos) {
    return 0;
  }
  idle_time = std::stoull(file_line);

  return (user_time + nice_time + system_time + idle_time);
}

static auto CpuProcOccupy(std::int32_t _pid) -> std::uint64_t {
#define PROCESS_ITEM 14
  // get specific pid cpu use time
  std::uint64_t user_time{};
  std::uint64_t system_time{};
  std::uint64_t cutime{};  // all user time
  std::uint64_t cstime{};  // all dead time
  std::ifstream file_stream{};
  std::string file_line{};
  std::string::size_type index{};

  file_stream.open(fmt::format("/proc/{}/stat", _pid));
  if (file_stream.fail()) {
    return 0;
  }

  std::getline(file_stream, file_line, '\n');
  file_stream.close();

  auto len = file_line.size();
  auto count{0};

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
  while (file_line[index++] == ' ') {
  }
  index = file_line.find(' ', index);
  if (index == std::string::npos) {
    return 0;
  }
  system_time = std::stoull(file_line);

  file_line = file_line.substr(index);
  while (file_line[index++] == ' ') {
  }
  index = file_line.find(' ', index);
  if (index == std::string::npos) {
    return 0;
  }
  cutime = std::stoull(file_line);

  file_line = file_line.substr(index);
  while (file_line[index++] == ' ') {
  }
  index = file_line.find(' ', index);
  if (index == std::string::npos) {
    return 0;
  }
  cstime = std::stoull(file_line);

  return (user_time + system_time + cutime + cstime);
#undef PROCESS_ITEM
}

static auto NProcs() -> std::int64_t { return ::sysconf(_SC_NPROCESSORS_ONLN); }

}  // namespace system_inner

auto SystemDir() -> std::string {
  return system_inner::SystemInfo::instance().system_dir_.data();
}

auto HostName() -> std::string {
  return system_inner::SystemInfo::instance().host_name_.data();
}

auto Pid() -> std::int32_t { return system_inner::SystemInfo::instance().pid_; }

auto PageSize() -> std::size_t {
  return system_inner::SystemInfo::instance().page_size_;
}

auto CpuUsage(std::int32_t _pid) -> double {
  auto total_cputime1 = system_inner::CpuTotalOccupy();
  auto pro_cputime1 = system_inner::CpuProcOccupy(_pid);

  // sleep 200ms to fetch two time point cpu usage snapshots sample for later
  // calculation.
  std::this_thread::sleep_for(
      std::chrono::microseconds(SLEEP_TIME_SLICE_MICROS));

  auto total_cputime2 = system_inner::CpuTotalOccupy();
  auto pro_cputime2 = system_inner::CpuProcOccupy(_pid);

  auto pcpu{0.0};
  if (0 != total_cputime2 - total_cputime1) {
    pcpu =
        static_cast<double>(pro_cputime2 - pro_cputime1) /
        static_cast<double>(total_cputime2 - total_cputime1);  // double number
  }

  auto cpu_num = system_inner::NProcs();
  pcpu *= static_cast<double>(
      cpu_num);  // should multiply cpu num in multiple cpu machine

  return pcpu;
}

auto StackTrace(bool _demangle) -> std::string {
  static const auto MAX_STACK_FRAMES = 20;
  static const auto DEMANGLE_SIZE = 256;

  std::string stack{};

  std::array<void*, MAX_STACK_FRAMES> frames{};
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
          demangled = ret;  // ret could be realloc()
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
}

auto SetCurrentThreadName(const char* _tname) -> bool {
  auto ret = -1;
  ret = ::prctl(PR_SET_NAME, _tname);
  return ret != -1;
}

auto ErrnoStr(std::int32_t _errorno) -> const char* {
  static thread_local std::array<char, HARE_SMALL_FIXED_SIZE *
                                           HARE_SMALL_FIXED_SIZE / 2>
      t_errno_buf;
  ::strerror_r(_errorno, t_errno_buf.data(), t_errno_buf.size());
  return t_errno_buf.data();
}

auto LocalAddress(std::uint8_t _family, std::list<std::string>& _addr_list)
    -> bool {
  // Get the list of ip addresses of machine
  ::ifaddrs* if_addrs{nullptr};
  auto ret = ::getifaddrs(&if_addrs);

  if (ret != 0) {
    return false;
  }

  auto adress_buf_len{0};
  std::array<char, INET6_ADDRSTRLEN> address_buffer{};

  switch (_family) {
    case AF_INET6:
      adress_buf_len = INET6_ADDRSTRLEN;
      break;
    case AF_INET:
      adress_buf_len = INET_ADDRSTRLEN;
      break;
    default:
      return false;
  }

  while (if_addrs != nullptr) {
    if (_family == if_addrs->ifa_addr->sa_family) {
      // Is a valid IPv4 Address
      auto* tmp = &(reinterpret_cast<struct sockaddr_in*>(if_addrs->ifa_addr))
                       ->sin_addr;
      ::inet_ntop(_family, tmp, address_buffer.data(), adress_buf_len);
      _addr_list.emplace_back(address_buffer.data(), adress_buf_len);
      address_buffer.fill('\0');
    }
    if_addrs = if_addrs->ifa_next;
  }
  return true;
}

}  // namespace hare

#endif