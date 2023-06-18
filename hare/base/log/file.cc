#include "hare/base/log/file.h"
#include <hare/base/logging.h>
#include <hare/base/util/system_info.h>

namespace hare {
namespace log {

    file::file(std::string _base_name,
        size_t _roll_size, bool _thread_safe,
        int32_t _flush_interval, int32_t _check_every_n)
        : base_name_(std::move(_base_name))
        , roll_size_(_roll_size)
        , flush_interval_(_flush_interval)
        , check_every_n_(_check_every_n)
        , mutex_(_thread_safe ? new std::mutex : nullptr)
    {
        HARE_ASSERT(base_name_.find('/') != std::string::npos, "cannot find \'/\'.");
        roll_file();
    }

    file::~file() = default;

    void file::append(const char* _log_line, size_t _len)
    {
        if (mutex_) {
            std::unique_lock<std::mutex> lock(*mutex_);
            append_unlocked(_log_line, _len);
        } else {
            append_unlocked(_log_line, _len);
        }
    }

    void file::flush()
    {
        if (mutex_) {
            std::unique_lock<std::mutex> lock(*mutex_);
            file_->flush();
        } else {
            file_->flush();
        }
    }

    void file::append_unlocked(const char* _log_line, size_t _len)
    {
        file_->append(_log_line, _len);

        if (file_->written_bytes() > roll_size_) {
            roll_file();
        } else {
            ++count_;
            if (count_ >= check_every_n_) {
                count_ = 0;
                auto now = ::time(nullptr);
                auto thisPeriod_ = now / SECONDS_PER_ROLL * SECONDS_PER_ROLL;
                if (thisPeriod_ != start_of_period_) {
                    roll_file();
                } else if (now - last_flush_ > flush_interval_) {
                    last_flush_ = now;
                    file_->flush();
                }
            }
        }
    }

    auto file::roll_file() -> bool
    {
        time_t now = 0;
        auto file_name = get_file_name(base_name_, &now);
        auto start = now / SECONDS_PER_ROLL * SECONDS_PER_ROLL;

        if (now > last_roll_) {
            last_roll_ = now;
            last_flush_ = now;
            start_of_period_ = start;
            file_.reset(new util::append_file(file_name));
            return true;
        }
        return false;
    }

    auto file::get_file_name(const std::string& _basename, time_t* _now) -> std::string
    {

        std::string file_name {};
        file_name.resize(_basename.size() + static_cast<uint64_t>(HARE_SMALL_FIXED_SIZE * 2));
        file_name = _basename;

        std::array<char, HARE_SMALL_FIXED_SIZE> cache {};
        struct tm stm { };
        *_now = ::time(nullptr);
        ::localtime_r(_now, &stm);
        auto ret = ::strftime(cache.data(), HARE_SMALL_FIXED_SIZE, ".%Y%m%d-%H%M%S.", &stm);
        file_name += cache.data();
        file_name += util::hostname();
        ret = ::snprintf(cache.data(), HARE_SMALL_FIXED_SIZE, ".%d", util::pid());
        file_name += cache.data();
        file_name += ".log";

        return file_name;
    }

} // namespace log
} // namespace hare
