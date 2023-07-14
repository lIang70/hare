/**
 * @file hare/base/util/system.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the functions associated with system.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_UTIL_SYSTEM_H_
#define _HARE_BASE_UTIL_SYSTEM_H_

#include <hare/base/fwd.h>

namespace hare {
namespace util {

    HARE_API auto system_dir() -> std::string;
    HARE_API auto hostname() -> std::string;
    HARE_API auto pid() -> std::int32_t;
    HARE_API auto page_size() -> std::size_t;
    HARE_API auto cpu_usage(std::int32_t _pid) -> double;
    HARE_API auto stack_trace(bool _demangle) -> std::string;
    HARE_API auto set_thread_name(const char* _tname) -> bool;
    HARE_API auto errnostr(std::int32_t _errorno) -> const char*;

    HARE_API auto open_s(std::FILE** _fp, const filename_t& _filename, const filename_t& _mode) -> bool;
    HARE_API auto fexists(const filename_t& _filepath) -> bool;
    HARE_API auto fsize(std::FILE* _fp) -> std::size_t;
    HARE_API auto fsync(std::FILE* _fp) -> bool;

} // namespace util
} // namespace hare

#endif // !_HARE_BASE_UTIL_SYSTEM_H_