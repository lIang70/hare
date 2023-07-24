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

#include <hare/base/util/queue.h>
#include <hare/log/backends/base_backend.h>
#include <hare/log/details/file.h>

namespace hare {
namespace log {

    namespace detail {

        template <std::uint64_t MaxSize = details::inline_buffer_size>
        struct rotate_file {
            std::int32_t rotate_id_ { 0 };
            std::int32_t max_files_ { 0 };

            explicit rotate_file(std::int32_t _max_files)
                : max_files_(_max_files)
            {
            }

            HARE_INLINE
            auto get_name(const filename_t& _basename) -> filename_t
            {
                auto tmp = rotate_id_++;
                if (max_files_ > 0) {
                    rotate_id_ %= max_files_;
                }
                return fmt::format("{}.{}.log", filename_to_str(_basename), rotate_id_);
            }

            template <bool WithLock>
            HARE_INLINE
            auto should_rotate(LEVEL _log_level, const details::file<WithLock>& _file) -> bool
            {
                ignore_unused(_log_level);
                return _file.size() >= MaxSize;
            }
        };

    } // detail

    template <typename Mutex, typename FileNameGenerator>
    class file_backend final : public base_backend<Mutex> {
        FileNameGenerator generator_ {};
        filename_t basename_ {};
        details::file<false> file_ {};
        std::int32_t max_files_ { -1 };
        util::circular_queue<filename_t> filename_history_ {};

    public:
        explicit file_backend(filename_t _basename, std::int32_t _max_files = -1, bool _rotate = true)
            : basename_(std::move(_basename))
            , generator_(_max_files)
            , max_files_(_max_files)
            , filename_history_(_max_files > 0 ? _max_files : 0)
        {
            file_.open(generator_.get_name(basename_));
            if (_max_files > 0) {
                filename_history_.push_back(file_.name());
            }
        }

        HARE_INLINE
        auto filename() const -> filename_t
        {
            std::lock_guard<Mutex> lock(base_backend<Mutex>::mutex_);
            return file_.name();
        }

    private:
        void inner_sink_it(details::msg_buffer_t& _msg, LEVEL _log_level) final
        {
            file_.append(_msg);

            auto should_rotation = generator_.should_rotate(_log_level, file_);
            if (should_rotation) {
                auto filename = generator_.get_name(basename_);
                file_.open(filename);
            }

            if (should_rotation && max_files_ > 0) {
                delete_old();
            }
        }

        void inner_flush() final
        {
            file_.flush();
        }

        void delete_old()
        {
            if (filename_history_.full()) {
                auto tmp = filename_history_.pop_front();
                if (!util::fremove(tmp)) {
                    fmt::print(stderr, "failed removing hourly file[{}]." HARE_EOL, tmp);
                }
            }
            filename_history_.push_back(file_.name());
        }
    };

    template <typename FileNameGenerator>
    using file_backend_mt = file_backend<std::mutex, FileNameGenerator>;

    template <typename FileNameGenerator>
    using file_backend_st = file_backend<details::dummy_mutex, FileNameGenerator>;

} // namespace log
} // namespace hare

#endif // _HARE_LOG_FILE_BACKEND_H_