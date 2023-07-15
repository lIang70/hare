/**
 * @file hare/base/registry.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with registry.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_REGISTRY_H_
#define _HARE_LOG_REGISTRY_H_

#include <hare/base/util/non_copyable.h>

namespace hare {
namespace log {

    HARE_CLASS_API
    class HARE_API registry : util::non_copyable {

    public:
        static auto instance() -> registry&;

        registry(registry&&) = delete;
        auto operator=(registry&&) -> registry& = delete;

    private:
        registry() = default;

    };

} // namespace log
} // namespace hare

#endif // _HARE_LOG_REGISTRY_H_