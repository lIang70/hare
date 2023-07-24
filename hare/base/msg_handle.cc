#include "hare/base/fwd-inl.h"

namespace hare {

namespace detail {
    void default_msg_handle(std::uint8_t _msg_type, const std::string& _msg)
    {
        static timestamp s_last_flush_time { timestamp::now() };
        fmt::print(stdout, _msg_type == TRACE_MSG ? "[trace] {}." : "[error] {}. \n" , _msg);
        auto tmp = timestamp::now();
        if (std::abs(timestamp::difference(tmp, s_last_flush_time) - 0.5) > (1e-5)) {
            s_last_flush_time.swap(tmp);
            ignore_unused(std::fflush(stdout));
        }
    }
} // namespace detail

void register_msg_handler(log_handler handle)
{
    msg() = std::move(handle);
}

} // namespace hare