/**
 * @file hare/base/util/system_info.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the functions associated with system_info.h
 * @version 0.1-beta
 * @date 2023-02-09
 * 
 * @copyright Copyright (c) 2023
 * 
 **/

#ifndef _HARE_BASE_UTIL_SYSTEM_INFO_H_
#define _HARE_BASE_UTIL_SYSTEM_INFO_H_

#include <hare/base/util.h>
#include <hare/base/error.h>

#include <string>

namespace hare {
namespace util {

    HARE_API auto system_dir() -> std::string;
    HARE_API auto pid() -> int32_t;
    HARE_API auto hostname() -> std::string;
    HARE_API auto cpu_usage(int32_t _pid) -> double;
    HARE_API auto stack_trace(bool demangle) -> std::string;
    HARE_API auto set_tname(const char* tname) -> error;

} // namespace util
} // namespace hare

#endif // !_HARE_BASE_UTIL_SYSTEM_INFO_H_