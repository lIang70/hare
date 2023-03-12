#ifndef _HARE_BASE_LOG_ASYNC_H_
#define _HARE_BASE_LOG_ASYNC_H_

#include <hare/base/util.h>

#include <cinttypes>
#include <string>

namespace hare {
namespace log {

    class HARE_API Async {
    public:
        // Flush every second.
        static const int32_t META_FLUSH_INTERVAL = 1;

    private:
        class Data;
        Data* d_ { nullptr };

    public:
        Async(const std::string& name, int64_t roll_size, int32_t flush_interval = 3 * META_FLUSH_INTERVAL);
        ~Async();

        void append(const char* log_line, int32_t size);
        void start();
        void stop();
    };

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_ASYNC_H_