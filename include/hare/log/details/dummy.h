/**
 * @file hare/log/details/dummy.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with dummy.h
 * @version 0.1-beta
 * @date 2023-07-13
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_DETAILS_DUMMY_H_
#define _HARE_LOG_DETAILS_DUMMY_H_

#include <hare/base/fwd.h>

namespace hare {
namespace log {
    namespace details {

        HARE_CLASS_API
        struct HARE_API dummy_mutex {
            void lock() { }
            void unlock() { }
            auto try_lock() -> bool
            {
                return true;
            }
        };

        template <typename T>
        struct dummy_atomic_t {
            using value = T;

            T value_ {};

            dummy_atomic_t() = default;

            explicit dummy_atomic_t(T _value)
                : value_(_value)
            {
            }

            auto load(std::memory_order = std::memory_order_relaxed) const -> T
            {
                return value_;
            }

            void store(T _value, std::memory_order = std::memory_order_relaxed)
            {
                value_ = _value;
            }
        };

    } // namespace details
} // namespace log
} // namespace hare

#endif // _HARE_LOG_DETAILS_DUMMY_H_