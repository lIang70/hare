#ifndef _HARE_BASE_LOG_FILE_H_
#define _HARE_BASE_LOG_FILE_H_

#include "hare/base/util/file.h"

#include <memory>
#include <mutex>

namespace hare {
namespace log {

    class File {
        const std::string base_name_ {};
        const std::size_t roll_size_ { 0 };
        const int32_t flush_interval_ { 0 };
        const int32_t check_every_n_ { 0 };

        int32_t count_ { 0 };

        std::unique_ptr<std::mutex> mutex_ { nullptr };
        time_t start_of_period_ { 0 };
        time_t last_roll_ { 0 };
        time_t last_flush_ { 0 };
        std::unique_ptr<util::AppendFile> file_ { nullptr };

    public:
        const static int32_t SECONDS_PER_ROLL = 60 * 60 * 24;
        const static int32_t CHECK_COUNT = 1024;

        File(const std::string& base_name,
            std::size_t roll_size,
            bool thread_safe = true,
            int32_t flush_interval = 3,
            int32_t check_every_n = CHECK_COUNT);
        ~File();

        void append(const char* log_line, int32_t len);
        void flush();
        auto rollFile() -> bool;

    private:
        void append_unlocked(const char* log_line, int32_t len);

        static auto getLogFileName(const std::string& basename, time_t* now) -> std::string;
    };

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_FILE_H_