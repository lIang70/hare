/**
 * @file hare/base/io/file.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with file.h
 * @version 0.2-beta
 * @date 2023-08-24
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_IO_FILE_H_
#define _HARE_BASE_IO_FILE_H_

#include <hare/base/exception.h>
#include <hare/base/util/non_copyable.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

#if !defined(HARE_EOL)
#ifdef H_OS_WIN32
#define HARE_EOL "\r\n"
#else
#define HARE_EOL "\n"
#endif
#endif

#if !defined(HARE_SLASH)
#ifdef H_OS_WIN32
#define HARE_SLASH '\\'
#else
#define HARE_SLASH '/'
#endif
#endif

#ifdef H_OS_WIN
#define fwrite_unlocked _fwrite_nolock
#endif

namespace hare {
namespace io {

    namespace file_inner {

        enum { BUFFER_SIZE = 64 * 1024 };

        struct WriteUnlock {
            auto operator()(const void* _ptr, std::size_t _size,
                std::size_t _n, std::FILE* _s) -> std::size_t
            {
                return ::fwrite_unlocked(_ptr, _size, _n, _s);
            }
        };

        struct Write {
            auto operator()(const void* _ptr, std::size_t _size,
                std::size_t _n, std::FILE* _s) -> std::size_t
            {
                return std::fwrite(_ptr, _size, _n, _s);
            }
        };

        HARE_API auto Open(std::FILE** _fp, const filename_t& _filename, const filename_t& _mode) -> bool;

        HARE_API auto Exists(const filename_t& _filepath) -> bool;

        HARE_API auto Remove(const filename_t& _filepath) -> bool;

        HARE_API auto Size(std::FILE* _fp) -> std::uint64_t;

        HARE_API auto Sync(std::FILE* _fp) -> bool;

    } // namespace file_inner

    template <bool WithLock = false, std::size_t Size = file_inner::BUFFER_SIZE>
    class FileHelper : public util::NonCopyable {
        using InnerWrite = typename std::conditional<
            WithLock, file_inner::Write, file_inner::WriteUnlock>::type;

        std::FILE* fp_ {};
        std::size_t written_bytes_ { 0 };
        std::size_t size_ { 0 };
        filename_t filename_ {};

        InnerWrite inner_write_;
        fmt::basic_memory_buffer<char, Size> buffer_ {};

    public:
        ~FileHelper() { Close(); }

        auto written_bytes() const -> std::size_t { return written_bytes_; }

        auto filename() const -> filename_t { return filename_; }

        void Open(const filename_t& _file, bool truncate = false)
        {
            Close();
            const auto* mode = truncate ? HARE_FILENAME_T("wb") : HARE_FILENAME_T("ab");
            if (!file_inner::Open(&fp_, _file, mode)) {
                throw Exception("failed opening file " + FilenameToStr(_file) + " for writing");
            } else {
                filename_ = _file;
                IgnoreUnused(std::setvbuf(fp_, buffer_.data(), _IOFBF, Size));
                size_ = file_inner::Size(fp_);
            }
        }

        void Reopen(bool truncate = false)
        {
            if (filename_.empty()) {
                throw Exception("failed re-opening file - was not opened before");
            }
            Open(filename_, truncate);
        }

        void Close()
        {
            if (fp_ != nullptr) {
                IgnoreUnused(std::fclose(fp_));
                fp_ = nullptr;
            }
            written_bytes_ = 0;
        }

        auto Length() const -> std::size_t
        {
            return fp_ == nullptr ? 0 : size_ + written_bytes_;
        }

        void Flush()
        {
            IgnoreUnused(std::fflush(fp_));
        }

        auto Sync() -> bool
        {
            return file_inner::Sync(fp_);
        }

        template <std::size_t BufferSize = file_inner::BUFFER_SIZE>
        void Append(const fmt::basic_memory_buffer<char, BufferSize>& _buffer)
        {
            const auto buffer_size = _buffer.size();

            if (inner_write_(_buffer.data(), 1, buffer_size, fp_) != buffer_size) {
                throw Exception("Failed writing to file " + FilenameToStr(filename_));
            }
            written_bytes_ += buffer_size;
        }
    };

} // namespace io
} // namespace hare

#endif // _HARE_BASE_IO_FILE_H_