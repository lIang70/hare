/**
 * @file hare/log/details/file.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with file.h
 * @version 0.1-beta
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_DETAILS_FILE_H_
#define _HARE_LOG_DETAILS_FILE_H_

#include <hare/base/exception.h>
#include <hare/base/util/system.h>
#include <hare/log/details/msg.h>

#include <codecvt>

namespace hare {
namespace log {

    namespace details {

        enum { inline_buffer_size = 64 * 1024 };

        template <bool WithLock = false, std::size_t Size = inline_buffer_size>
        class file : public util::non_copyable {
            struct write_unlock {
                HARE_INLINE
                auto operator()(const void* _ptr, std::size_t _size,
                    std::size_t _n, std::FILE* _s) -> std::size_t
                {
#if defined(H_OS_WIN32)
                    return ::_fwrite_nolock(_ptr, _size, _n, _s);
#else
                    return ::fwrite_unlocked(_ptr, _size, _n, _s);
#endif
                }
            };

            struct write {
                HARE_INLINE
                auto operator()(const void* _ptr, std::size_t _size,
                    std::size_t _n, std::FILE* _s) -> std::size_t
                {
                    return std::fwrite(_ptr, _size, _n, _s);
                }
            };

            std::FILE* fp_ {};
            std::size_t written_bytes_ { 0 };
            std::size_t size_ { 0 };
            filename_t filename_ {};
            fmt::basic_memory_buffer<char, Size> buffer_ {};

        public:
            HARE_INLINE
            ~file()
            {
                close();
            }

            HARE_INLINE
            auto written_bytes() const -> std::size_t { return written_bytes_; }

            HARE_INLINE
            auto name() const -> filename_t { return filename_; }

            HARE_INLINE
            void open(const filename_t& _file, bool truncate = false)
            {
                close();
                const auto* mode = truncate ? HARE_FILENAME_T("wb") : HARE_FILENAME_T("ab");
                if (!util::open_s(&fp_, _file, mode)) {
                    throw exception("failed opening file " + filename_to_str(_file) + " for writing");
                } else {
                    filename_ = _file;
                    ignore_unused(std::setvbuf(fp_, buffer_.data(), _IOFBF, Size));
                    size_ = util::fsize(fp_);
                }
            }

            HARE_INLINE
            void reopen(bool truncate = false)
            {
                if (filename_.empty()) {
                    throw exception("failed re-opening file - was not opened before");
                }
                open(filename_, truncate);
            }

            HARE_INLINE
            void close()
            {
                if (fp_ != nullptr) {
                    ignore_unused(std::fclose(fp_));
                    fp_ = nullptr;
                }
                written_bytes_ = 0;
            }

            HARE_INLINE
            auto size() const -> std::size_t
            {
                return fp_ == nullptr ? 0 : size_ + written_bytes_;
            }

            HARE_INLINE
            void flush()
            {
                ignore_unused(std::fflush(fp_));
            }

            HARE_INLINE
            auto sync() -> bool
            {
                return util::fsync(fp_);
            }

            void append(const msg_buffer_t& _buffer)
            {
                const auto buffer_size = _buffer.size();

                typedef typename std::conditional<
                    WithLock, write, write_unlock>::type inner_write;

                if (inner_write()(_buffer.data(), 1, buffer_size, fp_) != buffer_size) {
                    throw exception("Failed writing to file " + filename_to_str(filename_));
                }
                written_bytes_ += buffer_size;
            }
        };

    } // namespace details
} // namespace log
} // namespace hare

#endif // _HARE_LOG_DETAILS_FILE_H_