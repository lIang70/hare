#include "hare/base/thread/local.h"

#include <array>
#include <algorithm>
#include <sstream>
#include <thread>

namespace hare {
namespace current_thread {

    namespace detail {
        static const auto hex { 16 };
        static const std::array<char, hex + 1> sc_digits_hex = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
        static_assert(sc_digits_hex.size() == hex + 1, "wrong number of digits_hex");

        void convertHex(std::string& buf, thread::id value)
        {
            buf.clear();

            do {
                auto lsd = static_cast<int32_t>(value % hex);
                value /= hex;
                buf.push_back(sc_digits_hex[lsd]);
            } while (value != 0);

            buf.push_back('x');
            buf.push_back('0');
            std::reverse(buf.begin(), buf.end());
        }
    } // namespace detail

    auto get_tds() -> TDS&
    {
        static thread_local TDS s_tds;
        return s_tds;
    }

    void cache_thread_id()
    {
        if (get_tds().tid == 0) {
            std::ostringstream oss;
            oss << std::this_thread::get_id();
            get_tds().tid_str = oss.str();
            get_tds().tid = std::stoull(get_tds().tid_str);
            detail::convertHex(get_tds().tid_str, get_tds().tid);
        }
    }

} // namespace current_thread
} // namespace hare