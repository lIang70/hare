#ifndef _HARE_BASE_LOG_FILE_H_
#define _HARE_BASE_LOG_FILE_H_

#include <hare/base/util/file.h>

#include <memory>
#include <mutex>

namespace hare {
namespace log {

    class file {
        const std::string base_name_ {};
        const size_t roll_size_ { 0 };
        const int32_t flush_interval_ { 0 };
        const int32_t check_every_n_ { 0 };

        int32_t count_ { 0 };

        ptr<std::mutex> mutex_ { nullptr };
        time_t start_of_period_ { 0 };
        time_t last_roll_ { 0 };
        time_t last_flush_ { 0 };
        ptr<util::append_file> file_ { nullptr };

    public:
        const static int32_t SECONDS_PER_ROLL = 60 * 60 * 24;
        const static int32_t CHECK_COUNT = 1024;

        file(std::string _base_name,
            size_t _roll_size,
            bool _thread_safe = true,
            int32_t _flush_interval = 3,
            int32_t _check_every_n = CHECK_COUNT);
        ~file();

        void append(const char* _log_line, size_t _len);
        void flush();
        auto roll_file() -> bool;

    private:
        void append_unlocked(const char* _log_line, size_t _len);

        static auto get_file_name(const std::string& _basename, time_t* _now) -> std::string;
    };

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_FILE_H_