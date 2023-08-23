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
    namespace detail {

        HARE_CLASS_API
        struct HARE_API DummyMutex {
            virtual ~DummyMutex() = default;
            void lock() { }
            void unlock() { }
            virtual auto try_lock() -> bool
            {
                return true;
            }
        };

        HARE_CLASS_API
        template <typename T>
        struct HARE_API DummyAtomic {
            using value = T;

            T value_store {};

            DummyAtomic() = default;

            explicit DummyAtomic(T _value)
                : value_store(_value)
            {
            }

            auto load(std::memory_order = std::memory_order_relaxed) const -> T
            {
                return value_store;
            }

            void store(T _value, std::memory_order = std::memory_order_relaxed)
            {
                value_store = _value;
            }
        };

    } // namespace detail
} // namespace log
} // namespace hare

#endif // _HARE_LOG_DETAILS_DUMMY_H_
