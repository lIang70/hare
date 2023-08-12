/**
 * @file hare/base/util/count_down_latch.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with count_down_latch.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_COUNT_DOWN_LATCH_H_
#define _HARE_BASE_COUNT_DOWN_LATCH_H_

#include <hare/base/fwd.h>

namespace hare {
namespace util {

    HARE_CLASS_API
    class HARE_API CountDownLatch {
        hare::detail::Impl* impl_ {};

    public:
        explicit CountDownLatch(std::uint32_t count);
        ~CountDownLatch();

        void CountDown();
        void Await(std::int32_t milliseconds = 0);
        auto Count() -> std::uint32_t;
    };

} // namespace util
} // namespace hare

#endif // _HARE_BASE_COUNT_DOWN_LATCH_H_
