#include "hare/base/thread/local.h"
#include <hare/base/exception.h>

#include <utility>

namespace hare {

class Exception::Data {
public:
    std::string what_ {};
    std::string stack_ {};

    explicit Data(std::string  what)
        : what_(std::move(what))
    {
    }
};

Exception::Exception(std::string what)
    : d_(new Data(std::move(what)))
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