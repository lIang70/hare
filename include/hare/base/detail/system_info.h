#ifndef _HARE_BASE_UTIL_SYSTEM_INFO_H_
#define _HARE_BASE_UTIL_SYSTEM_INFO_H_

#include <inttypes.h>
#include <string>

namespace hare {
namespace util {

    extern std::string systemdir();
    extern int32_t pid();
    extern std::string hostname();

} // namespace util
} // namespace hare

#endif // !_HARE_BASE_UTIL_SYSTEM_INFO_H_