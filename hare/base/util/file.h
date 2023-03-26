#ifndef _HARE_BASE_FILE_H_
#define _HARE_BASE_FILE_H_

#include <hare/base/util/non_copyable.h>

#include <cstdint>
#include <string>

namespace hare {
namespace util {

    class AppendFile : public NonCopyable {
    public:
        static const int BUFFER_SIZE = 64 * 1024;

    private:
        FILE* fp_ { nullptr };
        char buffer_[BUFFER_SIZE] {};
        int64_t written_bytes_ { 0 };

    public:
        explicit AppendFile(const std::string& file_name);
        ~AppendFile();

        void append(const char* logline, std::size_t len);

        void flush();

        inline auto writtenBytes() const -> int64_t { return written_bytes_; }

    private:
        auto write(const char* log_line, std::size_t len) -> std::size_t;
    };

} // namespace util
} // namespace hare

#endif // !_HARE_BASE_FILE_H_