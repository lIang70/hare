#include <hare/base/system.h>

#include "base/fwd_inl.h"

#if !defined(HARE_EOL)
#ifdef H_OS_WIN32
#define EOL "\r\n"
#else
#define EOL "\n"
#endif
#endif

namespace hare {

namespace detail {
void DefaultMsgHandle(std::uint8_t _msg_type, const std::string& _msg) {
  static Timestamp s_last_flush_time{Timestamp::Now()};
  fmt::print(stdout,
             _msg_type == TRACE_MSG ? "[trace] {}. {}" : "[error] {}. {}", _msg,
             EOL);
  auto tmp = Timestamp::Now();
  if (std::abs(Timestamp::Difference(tmp, s_last_flush_time) - 0.5) > (1e-5)) {
    s_last_flush_time.Swap(tmp);
    IgnoreUnused(std::fflush(stdout));
  }
}
}  // namespace detail

void Abort(const char* _errmsg) {
#if defined(H_OS_WIN)
  //  Raise STATUS_FATAL_APP_EXIT.
  ULONG_PTR extra_info[1];
  extra_info[0] = (ULONG_PTR)errmsg_;
  ::RaiseException(0x40000015, EXCEPTION_NONCONTINUABLE, 1, extra_info);
#else
  IgnoreUnused(_errmsg);

#ifdef HARE_DEBUG
  fmt::print(stderr, "{}\n", ::hare::StackTrace(true));
#endif

  std::abort();
#endif
}

void RegisterLogHandler(LogHandler handle) { InnerLog().Store(handle); }

}  // namespace hare