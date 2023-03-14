#include "hare/base/thread/local.h"
#include <hare/base/exception.h>

#include <utility>

namespace hare {

Exception::Exception(std::string what)
    : what_(std::move(what))
    , stack_(current_thread::stackTrace(false))
{
}

Exception::~Exception() = default;

const char* Exception::what() const noexcept
{
    return what_.c_str();
}

const char* Exception::stackTrace() const noexcept
{
    return stack_.c_str();
}

}