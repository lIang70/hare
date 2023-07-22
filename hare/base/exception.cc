#include <hare/base/exception.h>
#include <hare/base/util/system.h>

namespace hare {

HARE_IMPL_DEFAULT(exception,
    std::string what_ {};
    std::string stack_ {};
)

exception::exception(std::string what) noexcept
    : impl_(new exception_impl)
{
    d_ptr(impl_)->what_ = std::move(what);
    d_ptr(impl_)->stack_ = util::stack_trace(false);
}

exception::~exception() noexcept
{
    delete impl_;
}

auto exception::what() const noexcept -> const char*
{
    return d_ptr(impl_)->what_.c_str();
}

auto exception::stack_trace() const noexcept -> const char*
{
    return d_ptr(impl_)->stack_.c_str();
}

} // namespace hare
