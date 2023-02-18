#ifndef _HARE_BASE_UTIL_SYSTEM_INFO_H_
#define _HARE_BASE_UTIL_SYSTEM_INFO_H_

#include <hare/base/util.h>

#include <cinttypes>
#include <string>

namespace hare {
namespace util {

    HARE_API std::string systemdir();
    HARE_API int32_t pid();
    HARE_API std::string hostname();

} // namespace util
} // namespace hare

#endif // !_HARE_BASE_UTIL_SYSTEM_INFO_H_