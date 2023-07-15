#include <hare/log/registry.h>

namespace hare {
namespace log {

    auto registry::instance() -> registry&
    {
        static registry s_registry {};
        return s_registry;
    }

} // namespace log
} // namespace hare