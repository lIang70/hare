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
    class HARE_API count_down_latch {
        hare::detail::impl* impl_ {};

    public:
        explicit count_down_latch(std::uint32_t count);
        ~count_down_latch();

        void count_down();
        void await(std::int32_t milliseconds = 0);
        auto count() -> std::uint32_t;
    };

} // namespace util
} // namespace hare

#endif // _HARE_BASE_COUNT_DOWN_LATCH_H_
