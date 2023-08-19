#include "base/fwd-inl.h"

#if !defined(HARE_EOL)
#ifdef H_OS_WIN32
#define EOL "\r\n"
#else
#define EOL "\n"
#endif
#endif

namespace hare {

namespace detail {
    void DefaultMsgHandle(std::uint8_t _msg_type, const std::string& _msg)
    {
        static Timestamp s_last_flush_time { Timestamp::Now() };
        fmt::print(stdout, _msg_type == TRACE_MSG ? "[trace] {}. {}": "[error] {}. {}", _msg, EOL);
        auto tmp = Timestamp::Now();
        if (std::abs(Timestamp::Difference(tmp, s_last_flush_time) - 0.5) > (1e-5)) {
            s_last_flush_time.Swap(tmp);
            IgnoreUnused(std::fflush(stdout));
        }
    }
} // namespace detail

void RegisterLogHandler(LogHandler handle)
{
    InnerLog() = std::move(handle);
}

} // namespace hare