#include "hare/base/util/file.h"
#include <hare/base/exception.h>
#include <hare/base/logging.h>
#include <hare/base/util.h>

#include <stdio.h>

#ifdef H_OS_WIN32
#include <Windows.h>
#include <io.h>
#elif defined(H_OS_LINUX)
#include <dirent.h>
#endif

namespace hare {
namespace util {

    AppendFile::AppendFile(const std::string& file_name)
        : fp_(::fopen(file_name.c_str(), "ae")) // 'e' for O_CLOEXEC
    {
        if (!fp_)
            throw Exception("Cannot open file[" + file_name + "].");
        ::setbuffer(fp_, buffer_, sizeof(buffer_)); // posix_fadvise POSIX_FADV_DONTNEED ?
    }

    AppendFile::~AppendFile()
    {
        ::fclose(fp_);
    }

    void AppendFile::append(const char* log_line, const size_t len)
    {
        auto written = 0;

        while (written != len) {
            auto remain = len - written;
            auto n = write(log_line + written, remain);
            if (n != remain) {
                if (auto err = ::ferror(fp_)) {
                    fprintf(stderr, "AppendFile::append() failed %s\n", log::strErrorno(err));
                    break;
                }
            }
            written += n;
        }

        written_bytes_ += written;
    }

    void AppendFile::flush()
    {
        ::fflush(fp_);
    }

    size_t AppendFile::write(const char* logline, size_t len)
    {
        return ::fwrite_unlocked(logline, 1, len, fp_);
    }

} // namespace util
} // namespace hare
