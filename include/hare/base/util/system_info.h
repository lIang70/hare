#ifndef _HARE_BASE_UTIL_SYSTEM_INFO_H_
#define _HARE_BASE_UTIL_SYSTEM_INFO_H_

#include <hare/base/util/util.h>

#include <cinttypes>
#include <list>
#include <string>

namespace hare {
namespace util {

    HARE_API auto systemDir() -> std::string;
    HARE_API auto pid() -> int32_t;
    HARE_API auto hostname() -> std::string;

} // namespace util
} // namespace hare

#endif // !_HARE_BASE_UTIL_SYSTEM_INFO_H_