//! 
//! @file hare/base/util/system_info.h
//! @author l1ang70 (gog_017@outlook.com)
//! @brief Describe the functions associated with
//!   system info.
//! @version 0.1-beta
//! @date 2023-02-09
//! 
//! @copyright Copyright (c) 2023
//! 

#ifndef _HARE_BASE_UTIL_SYSTEM_INFO_H_
#define _HARE_BASE_UTIL_SYSTEM_INFO_H_

#include <hare/base/util/util.h>

#include <cinttypes>
#include <string>

namespace hare {
namespace util {

    HARE_API auto systemDir() -> std::string;
    HARE_API auto pid() -> int32_t;
    HARE_API auto hostname() -> std::string;

} // namespace util
} // namespace hare

#endif // !_HARE_BASE_UTIL_SYSTEM_INFO_H_