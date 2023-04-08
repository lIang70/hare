#include <hare/base/exception.h>

#include "hare/base/thread/local.h"

namespace hare {

Exception::Exception(std::string what)
    : what_(std::move(what))
    , stack_(current_thread::stackTrace(false))
{
}

Exception::~Exception() = default;

auto Exception::what() const noexcept -> const char*
{
    return what_.c_str();
}

auto Exception::stackTrace() const noexcept -> const char*
{
    return stack_.c_str();
}

} // namespace hare