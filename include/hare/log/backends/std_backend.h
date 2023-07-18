/**
 * @file hare/log/backends/std_backend.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with std_backend.h
 * @version 0.1-beta
 * @date 2023-07-18
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_STD_BACKEND_H_
#define _HARE_LOG_STD_BACKEND_H_

#include <hare/base/fwd.h>
#include <hare/log/backends/base_backend.h>

namespace hare {
namespace log {

    template <typename Mutex>
    class std_backend final : public base_backend<Mutex> {

    public:
        using current_backend = std_backend<Mutex>;

        static auto instance() -> ptr<current_backend>
        {
            static ptr<current_backend> s_std_backend { new current_backend };
            return s_std_backend;
        }

    private:
        std_backend() = default;

        void inner_sink_it(details::msg_buffer_t& _msg, LEVEL _log_level) final
        {
            ignore_unused(std::fwrite(_msg.data(), 1, _msg.size(), _log_level <= LEVEL_INFO ? stdout : stderr));
            inner_flush();
        }

        void inner_flush() final
        {
            ignore_unused(std::fflush(stderr));
            ignore_unused(std::fflush(stdout));
        }
    };

    using std_backend_mt = std_backend<std::mutex>;
    using std_backend_st = std_backend<details::dummy_mutex>;

} // namespace log
} // namespace hare

#endif // _HARE_LOG_FILE_BACKEND_H_