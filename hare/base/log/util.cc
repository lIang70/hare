#include "hare/base/log/util.h"
#include <hare/base/logging.h>

namespace hare {
namespace log {

    /*
        Format a number with 5 characters, including SI units.
        [0,     999]
        [1.00k, 999k]
        [1.00M, 999M]
        [1.00G, 999G]
        [1.00T, 999T]
        [1.00P, 999P]
        [1.00E, inf)
    */
    auto formatSI(int64_t num) -> std::string
    {
        auto nnum = static_cast<double>(num);
        char buf[HARE_SMALL_FIXED_SIZE * 2];
        if (num < 1000) {
            snprintf(buf, sizeof(buf), "%" PRId64, num);
        } else if (num < 9995) {
            snprintf(buf, sizeof(buf), "%.2fk", nnum / 1e3);
        } else if (num < 99950) {
            snprintf(buf, sizeof(buf), "%.1fk", nnum / 1e3);
        } else if (num < 999500) {
            snprintf(buf, sizeof(buf), "%.0fk", nnum / 1e3);
        } else if (num < 9995000) {
            snprintf(buf, sizeof(buf), "%.2fM", nnum / 1e6);
        } else if (num < 99950000) {
            snprintf(buf, sizeof(buf), "%.1fM", nnum / 1e6);
        } else if (num < 999500000) {
            snprintf(buf, sizeof(buf), "%.0fM", nnum / 1e6);
        } else if (num < 9995000000) {
            snprintf(buf, sizeof(buf), "%.2fG", nnum / 1e9);
        } else if (num < 99950000000) {
            snprintf(buf, sizeof(buf), "%.1fG", nnum / 1e9);
        } else if (num < 999500000000) {
            snprintf(buf, sizeof(buf), "%.0fG", nnum / 1e9);
        } else if (num < 9995000000000) {
            snprintf(buf, sizeof(buf), "%.2fT", nnum / 1e12);
        } else if (num < 99950000000000) {
            snprintf(buf, sizeof(buf), "%.1fT", nnum / 1e12);
        } else if (num < 999500000000000) {
            snprintf(buf, sizeof(buf), "%.0fT", nnum / 1e12);
        } else if (num < 9995000000000000) {
            snprintf(buf, sizeof(buf), "%.2fP", nnum / 1e15);
        } else if (num < 99950000000000000) {
            snprintf(buf, sizeof(buf), "%.1fP", nnum / 1e15);
        } else if (num < 999500000000000000) {
            snprintf(buf, sizeof(buf), "%.0fP", nnum / 1e15);
        } else {
            snprintf(buf, sizeof(buf), "%.2fE", nnum / 1e18);
        }
        return buf;
    }

    /*
        [0, 1023]
        [1.00Ki, 9.99Ki]
        [10.0Ki, 99.9Ki]
        [ 100Ki, 1023Ki]
        [1.00Mi, 9.99Mi]
    */
    auto formatIEC(int64_t num) -> std::string
    {
        auto nnum = static_cast<double>(num);
        char buf[HARE_SMALL_FIXED_SIZE * 2];
        const double Ki = 1024.0;
        const double Mi = Ki * 1024.0;
        const double Gi = Mi * 1024.0;
        const double Ti = Gi * 1024.0;
        const double Pi = Ti * 1024.0;
        const double Ei = Pi * 1024.0;

        if (nnum < Ki)
            snprintf(buf, sizeof(buf), "%" PRId64, num);
        else if (nnum < Ki * 9.995)
            snprintf(buf, sizeof(buf), "%.2fKi", nnum / Ki);
        else if (nnum < Ki * 99.95)
            snprintf(buf, sizeof(buf), "%.1fKi", nnum / Ki);
        else if (nnum < Ki * 1023.5)
            snprintf(buf, sizeof(buf), "%.0fKi", nnum / Ki);

        else if (nnum < Mi * 9.995)
            snprintf(buf, sizeof(buf), "%.2fMi", nnum / Mi);
        else if (nnum < Mi * 99.95)
            snprintf(buf, sizeof(buf), "%.1fMi", nnum / Mi);
        else if (nnum < Mi * 1023.5)
            snprintf(buf, sizeof(buf), "%.0fMi", nnum / Mi);

        else if (nnum < Gi * 9.995)
            snprintf(buf, sizeof(buf), "%.2fGi", nnum / Gi);
        else if (nnum < Gi * 99.95)
            snprintf(buf, sizeof(buf), "%.1fGi", nnum / Gi);
        else if (nnum < Gi * 1023.5)
            snprintf(buf, sizeof(buf), "%.0fGi", nnum / Gi);

        else if (nnum < Ti * 9.995)
            snprintf(buf, sizeof(buf), "%.2fTi", nnum / Ti);
        else if (nnum < Ti * 99.95)
            snprintf(buf, sizeof(buf), "%.1fTi", nnum / Ti);
        else if (nnum < Ti * 1023.5)
            snprintf(buf, sizeof(buf), "%.0fTi", nnum / Ti);

        else if (nnum < Pi * 9.995)
            snprintf(buf, sizeof(buf), "%.2fPi", nnum / Pi);
        else if (nnum < Pi * 99.95)
            snprintf(buf, sizeof(buf), "%.1fPi", nnum / Pi);
        else if (nnum < Pi * 1023.5)
            snprintf(buf, sizeof(buf), "%.0fPi", nnum / Pi);

        else if (nnum < Ei * 9.995)
            snprintf(buf, sizeof(buf), "%.2fEi", nnum / Ei);
        else
            snprintf(buf, sizeof(buf), "%.1fEi", nnum / Ei);
        return buf;
    }

} // namespace log
} // namespace hare
