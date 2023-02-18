#include "hare/base/thread/local.h"
#include <hare/base/exception.h>

namespace hare {

class Exception::Data {
public:
    std::string what_ {};
    std::string stack_ {};

    Data(const std::string& what)
        : what_(what)
    {
    }
};

Exception::Exception(std::string what)
    : d_(new Data(what))
{
    d_->stack_ = current_thread::stackTrace(false);
}

Exception::~Exception()
{
    delete d_;
}

const char* Exception::what() const noexcept
{
    return d_->what_.c_str();
}

const char* Exception::stackTrace() const noexcept
{
    return d_->stack_.c_str();
}

}