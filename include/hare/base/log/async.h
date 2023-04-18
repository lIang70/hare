/**
 * @file hare/base/log/async.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with async.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_LOG_ASYNC_H_
#define _HARE_BASE_LOG_ASYNC_H_

#include <hare/base/util/buffer.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/base/thread/thread.h>

#include <string>
#include <thread>
#include <vector>

namespace hare {
namespace log {

    class HARE_API async : public thread {
        // Flush every second.
        static const int32_t META_FLUSH_INTERVAL = 1;
        static const int64_t BLOCK_NUMBER = 16;

        using fixed_block = util::fixed_buffer<HARE_LARGE_BUFFER>;
        using blocks = std::vector<fixed_block::ptr>;

        std::mutex mutex_ {};
        std::condition_variable cv_ {};
        fixed_block::ptr current_block_ { new fixed_block };
        fixed_block::ptr next_block_ { new fixed_block };
        blocks blocks_ {};

        bool running_ { false };
        std::string name_ {};
        int64_t roll_size_ { static_cast<int64_t>(HARE_LARGE_BUFFER) * BLOCK_NUMBER };
        int32_t flush_interval_ { META_FLUSH_INTERVAL };

    public:
        using ptr = hare::ptr<async>;

        async(int64_t roll_size, std::string name, int32_t flush_interval = 3 * META_FLUSH_INTERVAL);
        ~async();

        void append(const char* log_line, int32_t size);
        void stop();

    private:
        void run();
    };

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_ASYNC_H_