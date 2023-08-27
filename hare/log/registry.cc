#include "base/fwd-inl.h"
#include <hare/log/registry.h>
#include <hare/base/exception.h>

namespace hare {
namespace log {

    auto Registry::Instance() -> Registry&
    {
        static Registry s_registry {};
        return s_registry;
    }

    void Registry::AssertExists(const std::string& logger_name)
    {
        if (loggers_.find(logger_name) != loggers_.end()) {
            HARE_INTERNAL_FATAL("logger with name '" + logger_name + "' already exists.");
        }
    }

} // namespace log
} // namespace hare