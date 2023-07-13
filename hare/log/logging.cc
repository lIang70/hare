#include "hare/base/fwd-inl.h"
#include <hare/log/logging.h>

#include <utility>

namespace hare {
namespace log {

    namespace detail {
        HARE_INLINE
        void handle_logger_error(const std::string& error_msg)
        {
            MSG_ERROR("{}", error_msg);
        }
    } // namespace deatil

    logger::logger(std::string _unique_name, backend_list _backends)
        : name_(std::move(_unique_name))
        , error_handle_(detail::handle_logger_error)
        , backends_(std::move(_backends))
    {
    }

    logger::logger(std::string _unique_name, ptr<backend> _backend)
        : name_(std::move(_unique_name))
        , error_handle_(detail::handle_logger_error)
        , backends_({ std::move(_backend) })
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
            error_handle_(e.what());
        } catch (...) {
            error_handle_("Unknown exeption in logger");
        }
    }

    void logger::sink_it(details::msg& _msg)
    {
        incr_msg_id(_msg);

        for (auto& backend : backends_) {
            if (backend->check(_msg.level_)) {
                backend->log(_msg);
            }
        }

        if (should_flush_on(_msg)) {
            flush();
        }
    }

} // namespace log
} // namespace hare