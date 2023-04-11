/**
 * @file hare/base/log/async.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with async.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef _HARE_BASE_LOG_ASYNC_H_
#define _HARE_BASE_LOG_ASYNC_H_

#include <hare/base/log/stream.h>
#include <hare/base/util/count_down_latch.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace hare {
namespace log {

    class HARE_API Async {
    public:
        // Flush every second.
        static const int32_t META_FLUSH_INTERVAL = 1;
        static const int64_t BLOCK_NUMBER = 16;

    private:
        using Block = detail::FixedBuffer<LARGE_BUFFER>;
        using Blocks = std::vector<Block::Ptr>;

        std::shared_ptr<std::thread> thread_;
        util::CountDownLatch latch_ { 1 };
        std::mutex mutex_ {};
        std::condition_variable cv_ {};
        Block::Ptr current_block_ { new Block };
        Block::Ptr next_block_ { new Block };
        Blocks blocks_ {};

        std::atomic<bool> running_ { false };
        std::string name_ {};
        int64_t roll_size_ { LARGE_BUFFER * BLOCK_NUMBER };
        int32_t flush_interval_ { META_FLUSH_INTERVAL };

    public:
        using Ptr = std::shared_ptr<Async>;

        Async(int64_t roll_size, std::string name, int32_t flush_interval = 3 * META_FLUSH_INTERVAL);
        ~Async();

        void append(const char* log_line, int32_t size);
        void start();
        void stop();

    private:
        void run();
    };

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_ASYNC_H_