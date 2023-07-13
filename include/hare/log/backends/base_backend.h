/**
 * @file hare/log/backends/base_backend.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with base_backend.h
 * @version 0.1-beta
 * @date 2023-07-12
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_BACKEND_BASE_H_
#define _HARE_LOG_BACKEND_BASE_H_

#include <hare/log/details/msg.h>

#include <atomic>
#include <mutex>

namespace hare {
namespace log {

    using POLICY = enum {
        POLICY_BLOCK_RETRY, // Block / yield / sleep until message can be enqueued
        POLICY_DISCARD // Discard the message it enqueue fails
    };

    HARE_CLASS_API
    class HARE_API backend {
        std::atomic<std::int8_t> level_ {};

    public:
        virtual ~backend() = default;

        virtual void log(const details::msg& msg) = 0;
        virtual void flush() = 0;

        HARE_INLINE
        auto check(LEVEL _msg_level) const -> bool
        {
            return _msg_level < level_.load(std::memory_order_relaxed);
        }

        HARE_INLINE
        void set_level(LEVEL log_level)
        {
            level_.store(log_level);
        }

        HARE_INLINE
        auto level() const -> LEVEL
        {
            return static_cast<LEVEL>(level_.load(std::memory_order_relaxed));
        }
    };

    template <typename Mutex = details::dummy_mutex>
    class base_backend : public backend
                       , public util::non_copyable {
    protected:
        Mutex mutex_ {};

    public:
        base_backend() = default;

        void log(const details::msg& _msg) final
        {
            std::lock_guard<Mutex> lock(mutex_);
            inner_sink_it(_msg);
        }

        void flush() final
        {
            std::lock_guard<Mutex> lock(mutex_);
            inner_flush();
        }

    protected:
        virtual void inner_sink_it(const details::msg& _msg) = 0;
        virtual void inner_flush() = 0;
    };

} // namespace log
} // namespace hare

#endif // _HARE_LOG_BACKEND_BASE_H_