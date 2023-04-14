/**
 * @file hare/base/util/non_copyable.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with non_copyable.h
 * @version 0.1-beta
 * @date 2023-02-09
 * 
 * @copyright Copyright (c) 2023
 * 
 **/

#ifndef _HARE_BASE_NON_COPYABLE_H_
#define _HARE_BASE_NON_COPYABLE_H_

#include <hare/base/util.h>

namespace hare {

class HARE_API non_copyable {
public:
    non_copyable(const non_copyable&) = delete;
    auto operator=(const non_copyable&) -> non_copyable& = delete;

protected:
    non_copyable() = default;
    ~non_copyable() = default;
};

} // namespace hare

#endif // !_HARE_BASE_NON_COPYABLE_H_