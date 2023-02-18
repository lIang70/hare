#ifndef _HARE_BASE_LOG_UTIL_H_
#define _HARE_BASE_LOG_UTIL_H_

#include <hare/base/util.h>

#include <string>

namespace hare {
namespace log {

    // Format quantity n in SI units (k, M, G, T, P, E).
    // The returned string is atmost 5 characters long.
    // Requires n >= 0
    std::string formatSI(int64_t n);

    // Format quantity n in IEC (binary) units (Ki, Mi, Gi, Ti, Pi, Ei).
    // The returned string is atmost 6 characters long.
    // Requires n >= 0
    std::string formatIEC(int64_t n);

} // namespace log
} // namespace hare

#endif // !_HARE_BASE_LOG_UTIL_H_