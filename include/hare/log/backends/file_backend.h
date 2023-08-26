/**
 * @file hare/log/backends/file_backend.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with file_backend.h
 * @version 0.1-beta
 * @date 2023-07-12
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_FILE_BACKEND_H_
#define _HARE_LOG_FILE_BACKEND_H_

#include <hare/base/io/file.h>
#include <hare/base/util/queue.h>
#include <hare/log/backends/base_backend.h>

namespace hare {
namespace log {

    namespace detail {

        template <std::uint64_t MaxSize = io::file_inner::BUFFER_SIZE>
        struct RotateFileBySize {
            std::int32_t rotate_id_ { 0 };
            std::int32_t max_files_ { 0 };

            explicit RotateFileBySize(std::int32_t _max_files)
                : max_files_(_max_files)
            {
            }

            HARE_INLINE
            auto GetName(const filename_t& _basename) -> filename_t
            {
                auto tmp = rotate_id_++;
                if (max_files_ > 0) {
                    rotate_id_ %= max_files_;
                }
                return fmt::format("{}.{}.log", FilenameToStr(_basename), tmp);
            }

            template <bool WithLock>
            HARE_INLINE auto ShouldRotate(Level _log_level, const io::FileHelper<WithLock>& _file) -> bool
            {
                IgnoreUnused(_log_level);
                return _file.Length() >= MaxSize;
            }
        };

    } // detail

    template <typename Mutex, typename FileNameGenerator>
    class FileBackend final : public BaseBackend<Mutex> {
        FileNameGenerator generator_ {};
        filename_t basename_ {};
        io::FileHelper<false> file_ {};
        std::int32_t max_files_ { -1 };
        util::CircularQueue<filename_t> filename_history_ {};

    public:
        explicit FileBackend(filename_t _basename, std::int32_t _max_files = -1, bool _rotate = true)
            : generator_(_max_files)
            , basename_(std::move(_basename))
            , max_files_(_max_files)
            , filename_history_(_max_files > 0 ? _max_files : 0)
        {
            file_.Open(generator_.GetName(basename_));
            if (_max_files > 0) {
                filename_history_.PushBack(file_.filename());
            }
        }

        HARE_INLINE
        auto filename() const -> filename_t
        {
            std::lock_guard<Mutex> lock(BaseBackend<Mutex>::mutex_);
            return file_.filename();
        }

    private:
        void InnerSinkIt(detail::msg_buffer_t& _msg, Level _log_level) final
        {
            file_.Append(_msg);

            auto should_rotation = generator_.ShouldRotate(_log_level, file_);
            if (should_rotation) {
                auto filename = generator_.GetName(basename_);
                file_.Open(filename);
            }

            if (should_rotation && max_files_ > 0) {
                DeleteOld();
            }
        }

        void InnerFlush() final
        {
            file_.Flush();
        }

        void DeleteOld()
        {
            if (filename_history_.Full()) {
                auto tmp = filename_history_.PopFront();
                if (!io::file_inner::Remove(tmp)) {
                    fmt::print(stderr, "failed removing hourly file[{}]." HARE_EOL, tmp);
                }
            }
            filename_history_.PushBack(file_.filename());
        }
    };

    template <typename FileNameGenerator>
    using FileBackendMT = FileBackend<std::mutex, FileNameGenerator>;

    template <typename FileNameGenerator>
    using FileBackendSt = FileBackend<detail::DummyMutex, FileNameGenerator>;

} // namespace log
} // namespace hare

#endif // _HARE_LOG_FILE_BACKEND_H_