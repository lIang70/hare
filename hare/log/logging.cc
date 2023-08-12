#include <hare/log/logging.h>

#include <utility>

namespace hare {
namespace log {

    namespace detail {
        void HandleLoggerError(std::uint8_t _msg_type, const std::string& _error_msg)
        {
            static Timestamp s_last_flush_time { Timestamp::Now() };
            fmt::print(stdout, _msg_type == TRACE_MSG ? "[trace] {}." : "[error] {}." HARE_EOL, _error_msg);
            auto tmp = Timestamp::Now();
            if (std::abs(Timestamp::Difference(tmp, s_last_flush_time) - 0.5) > (1e-5)) {
                s_last_flush_time.Swap(tmp);
                IgnoreUnused(std::fflush(stdout));
            }
        }
    } // namespace deatil

} // namespace log
} // namespace hare
