#include "hare/base/log/file.h"
#include <hare/base/detail/system_info.h>
#include <hare/base/logging.h>

namespace hare {
namespace log {

    File::File(const std::string& base_name,
        std::size_t roll_size, bool thread_safe,
        int32_t flush_interval, int32_t check_every_n)
        : base_name_(base_name)
        , roll_size_(roll_size)
        , flush_interval_(flush_interval)
        , check_every_n_(check_every_n)
        , count_(0)
        , mutex_(thread_safe ? new std::mutex : nullptr)
        , start_of_period_(0)
        , last_roll_(0)
        , last_flush_(0)
    {
        HARE_ASSERT(base_name.find('/') != std::string::npos, "Cannot find \'/\'.");
        rollFile();
    }

    File::~File() = default;

    void File::append(const char* logline, int len)
    {
        if (mutex_) {
            std::unique_lock<std::mutex> lock(*mutex_);
            append_unlocked(logline, len);
        } else {
            append_unlocked(logline, len);
        }
    }

    void File::flush()
    {
        if (mutex_) {
            std::unique_lock<std::mutex> lock(*mutex_);
            file_->flush();
        } else {
            file_->flush();
        }
    }

    void File::append_unlocked(const char* logline, int len)
    {
        file_->append(logline, len);

        if (file_->writtenBytes() > roll_size_) {
            rollFile();
        } else {
            ++count_;
            if (count_ >= check_every_n_) {
                count_ = 0;
                auto now = ::time(nullptr);
                auto thisPeriod_ = now / SECONDS_PER_ROLL * SECONDS_PER_ROLL;
                if (thisPeriod_ != start_of_period_) {
                    rollFile();
                } else if (now - last_flush_ > flush_interval_) {
                    last_flush_ = now;
                    file_->flush();
                }
            }
        }
    }

    bool File::rollFile()
    {
        time_t now = 0;
        auto file_name = getLogFileName(base_name_, &now);
        auto start = now / SECONDS_PER_ROLL * SECONDS_PER_ROLL;

        if (now > last_roll_) {
            last_roll_ = now;
            last_flush_ = now;
            start_of_period_ = start;
            file_.reset(new util::AppendFile(file_name));
            return true;
        }
        return false;
    }

    std::string File::getLogFileName(const std::string& basename, time_t* now)
    {
        std::string file_name;
        file_name.reserve(basename.size() + 64);
        file_name = basename;

        char time_buf[32];
        struct tm tm;
        *now = ::time(nullptr);
        gmtime_r(now, &tm); // FIXME: localtime_r ?
        strftime(time_buf, sizeof(time_buf), ".%Y%m%d-%H%M%S.", &tm);
        file_name += time_buf;

        file_name += util::hostname();

        char pid_buf[32];
        snprintf(pid_buf, sizeof(pid_buf), ".%d", util::pid());
        file_name += pid_buf;
        file_name += ".log";

        return file_name;
    }

} // namespace log
} // namespace hare
