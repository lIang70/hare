/**
 * @file hare/base/util/file.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with file.h
 * @version 0.1-beta
 * @date 2023-04-12
 * 
 * @copyright Copyright (c) 2023
 * 
 **/

#ifndef _HARE_BASE_FILE_H_
#define _HARE_BASE_FILE_H_

#include <hare/base/util/buffer.h>

#include <string>

namespace hare {
namespace util {

    class HARE_API append_file : public non_copyable {
    public:
        static const int BUFFER_SIZE = 64 * 1024;

    private:
        FILE* fp_ { nullptr };
        fixed_buffer<BUFFER_SIZE> buffer_ {};
        size_t written_bytes_ { 0 };

    public:
        explicit append_file(const std::string& _file_name);
        ~append_file();

        void append(const char* _log_line, size_t _len);

        void flush();

        inline auto written_bytes() const -> size_t { return written_bytes_; }

    private:
        auto write(const char* _log_line, size_t _len) -> size_t;
    };

} // namespace util
} // namespace hare

#endif // !_HARE_BASE_FILE_H_