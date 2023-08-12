#include <hare/log/registry.h>
#include <hare/base/exception.h>

namespace hare {
namespace log {

    auto Registry::instance() -> Registry&
    {
        static Registry s_registry {};
        return s_registry;
    }

    void Registry::assert_if_exists(const std::string& logger_name)
    {
        if (loggers_.find(logger_name) != loggers_.end()) {
            throw Exception("logger with name '" + logger_name + "' already exists.");
        }
    }

} // namespace log
} // namespace hare