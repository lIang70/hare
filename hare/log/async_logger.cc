#include <hare/log/async_logger.h>

namespace hare {
namespace log {

    async_logger::~async_logger()
    {
        auto thr_cnt = thread_pool_.thr_num();
        for (auto i = 0; i < thr_cnt; ++i) {
            thread_pool_.post(
                details::async_msg(details::async_msg::terminate),
                POLICY::BLOCK_RETRY);
        }
        thread_pool_.join();
    }

    void async_logger::flush()
    {
        thread_pool_.post(
            details::async_msg(details::async_msg::flush),
            POLICY::BLOCK_RETRY);
    }

    void async_logger::sink_it(details::msg& _msg)
    {
        thread_pool_.post({ _msg, details::async_msg::log }, msg_policy_);
    }

    auto async_logger::handle_msg(details::async_msg& _msg) -> bool
    {
        switch (_msg.type_) {
        case details::async_msg::log:
            incr_msg_id(_msg);

            for (auto& backend : backends_) {
                if (backend->check(_msg.level_)) {
                    backend->log(_msg);
                }
            }

            if (should_flush_on(_msg)) {
                flush();
            }
            return true;
        case details::async_msg::flush:
            try {
                for (auto& backend : backends_) {
                    backend->flush();
                }
            } catch (const hare::exception& e) {
                error_handle_(e.what());
            } catch (...) {
                error_handle_("Unknown exeption in logger");
            }
            return true;
            break;
        case details::async_msg::terminate:
            break;
        default:
            assert(false);
            break;
        }
        return false;
    }

} // namespace log
} // namespace hare