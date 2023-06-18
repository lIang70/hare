#include <hare/base/util/file.h>

#include <hare/base/exception.h>
#include <hare/base/logging.h>

#include <cstdio>

#ifdef H_OS_WIN32
#else
#include <dirent.h>
#endif

namespace hare {
namespace util {

    append_file::append_file(const std::string& file_name)
#ifdef H_OS_WIN32
#else
        : fp_(::fopen(file_name.c_str(), "ae")) // 'e' for O_CLOEXEC
#endif
    {
        if (fp_ == nullptr) {
            throw exception("Cannot open file[" + file_name + "].");
        }
        ::setbuffer(fp_, buffer_.begin(), BUFFER_SIZE); // posix_fadvise POSIX_FADV_DONTNEED ?
    }

    append_file::~append_file()
    {
        auto ret = ::fclose(fp_);
        H_UNUSED(ret);
    }

    void append_file::append(const char* _log_line, size_t _len)
    {
        auto written = static_cast<size_t>(0);

        while (written != _len) {
            auto remain = _len - written;
            auto write_n = write(_log_line + written, remain);
            if (write_n != remain) {
                if (auto err = ::ferror(fp_)) {
                    auto ret = ::fprintf(stderr, "append_file::append() failed %s\n", log::errnostr(err));
                    H_UNUSED(ret);
                    break;
                }
            }
            written += write_n;
        }

        written_bytes_ += written;
    }

    void append_file::flush()
    {
        auto ret = ::fflush(fp_);
        H_UNUSED(ret);
    }

    auto append_file::write(const char* log_line, size_t len) -> size_t
    {
        return ::fwrite_unlocked(log_line, 1, len, fp_);
    }

} // namespace util
} // namespace hare
