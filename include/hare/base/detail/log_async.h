#ifndef _HARE_BASE_LOG_ASYNC_H_
#define _HARE_BASE_LOG_ASYNC_H_

#include <hare/base/detail/log_util.h>

#include <atomic>
#include <inttypes.h>
#include <memory>
#include <vector>

namespace hare {
namespace log {

    class Async {
    public:
        using Block = detail::FixedBuffer<LARGE_BUFFER>;
        using BlockV = std::vector<std::unique_ptr<Block>>;
        using BPtr = BlockV::value_type;

        // Flush every second.
        static const int32_t META_FLUSH_INTERVAL = 1;

    private:
        std::atomic_bool running_ { false };
        std::string name_ {};
        int32_t flush_interval_ { META_FLUSH_INTERVAL };
        int64_t roll_size_ { LARGE_BUFFER * 16 };

        class Data;
        std::shared_ptr<Data> data_ { nullptr };

    public:
        Async(const std::string& name, int64_t roll_size, int32_t flush_interval = 3 * META_FLUSH_INTERVAL);
        ~Async()
        {
            if (running_) {
                stop();
            }
        }

        void append(const char* log_line, int32_t size);
        void start();
        void stop();
    };

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_ASYNC_H_