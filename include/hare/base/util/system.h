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

#include <list>

namespace hare {
namespace util {

    HARE_API auto SystemDir() -> std::string;
    HARE_API auto HostName() -> std::string;
    HARE_API auto Pid() -> std::int32_t;
    HARE_API auto PageSize() -> std::size_t;
    HARE_API auto CpuUsage(std::int32_t _pid) -> double;
    HARE_API auto StackTrace(bool _demangle) -> std::string;
    HARE_API auto SetCurrentThreadName(const char* _tname) -> bool;
    HARE_API auto ErrnoStr(std::int32_t _errorno) -> const char*;

    HARE_API auto FileOpen(std::FILE** _fp, const filename_t& _filename, const filename_t& _mode) -> bool;
    HARE_API auto FileExists(const filename_t& _filepath) -> bool;
    HARE_API auto FileRemove(const filename_t& _filepath) -> bool;
    HARE_API auto FileSize(std::FILE* _fp) -> std::size_t;
    HARE_API auto FileSync(std::FILE* _fp) -> bool;

    HARE_API auto LocalAddress(std::uint8_t _family, std::list<std::string>& _addr_list) -> std::int32_t;

} // namespace util
} // namespace hare

#endif // _HARE_BASE_UTIL_SYSTEM_H_