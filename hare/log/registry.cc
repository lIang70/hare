#include <hare/log/registry.h>
#include <hare/base/exception.h>

namespace hare {
namespace log {

    auto registry::instance() -> registry&
    {
        static registry s_registry {};
        return s_registry;
    }

    void registry::assert_if_exists(const std::string& logger_name)
    {
        if (loggers_.find(logger_name) != loggers_.end()) {
            throw exception("logger with name '" + logger_name + "' already exists.");
        }
    }

} // namespace log
} // namespace hare