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

    logger::logger(std::string _unique_name, backend_list _backends)
        : logger(std::move(_unique_name), _backends.begin(), _backends.end())
    {
    }

    logger::logger(std::string _unique_name, ptr<backend> _backend)
        : logger(std::move(_unique_name), backend_list { std::move(_backend) })
    {
    }

    logger::~logger() = default;

    void logger::flush()
    {
        try {
            for (auto& backend : backends_) {
                backend->flush();
            }
        } catch (const hare::exception& e) {
            error_handle_(ERROR_MSG, e.what());
        } catch (const std::exception& e) {
            error_handle_(ERROR_MSG, e.what());
        } catch (...) {
            error_handle_(ERROR_MSG, "Unknown exeption in logger");
        }
    }

    void logger::sink_it(details::msg& _msg)
    {
        incr_msg_id(_msg);

        details::msg_buffer_t formatted {};
        details::format_msg(_msg, formatted);

        for (auto& backend : backends_) {
            if (backend->check(_msg.level_)) {
                backend->log(formatted, static_cast<LEVEL>(_msg.level_));
            }
        }

        if (should_flush_on(_msg)) {
            flush();
        }
    }

} // namespace log
} // namespace hare