#include <hare/log/logging.h>

#include <utility>

namespace hare {
namespace log {

    namespace detail {
        void handle_logger_error(std::uint8_t _msg_type, const std::string& _error_msg)
        {
            static timestamp s_last_flush_time { timestamp::now() };
            fmt::println(stdout, _msg_type == TRACE_MSG ? "[trace] {}." : "[error] {}.", _error_msg);
            auto tmp = timestamp::now();
            if (std::abs(timestamp::difference(tmp, s_last_flush_time) - 0.5) > (1e-5)) {
                s_last_flush_time.swap(tmp);
                ignore_unused(std::fflush(stdout));
            }
        }
    } // namespace deatil

} // namespace log
} // namespace hare
