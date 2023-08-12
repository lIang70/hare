#include "hare/base/fwd-inl.h"
#include <hare/base/exception.h>
#include <hare/base/util/system.h>

namespace hare {

namespace detail {

    HARE_IMPL_DEFAULT(
        Exception,
        std::string what {};
        std::string stack_trace {};)

    static auto ThreadException() -> ExceptionImpl*
    {
        static thread_local ExceptionImpl thread_exception_impl {};
        return &thread_exception_impl;
    }

} // namespace detail

Exception::Exception(std::string what) noexcept
    : impl_(detail::ThreadException())
{
    d_ptr(impl_)->what = std::move(what);
    d_ptr(impl_)->stack_trace = util::StackTrace(false);
}

Exception::~Exception() noexcept
{
    delete impl_;
}

auto Exception::what() const noexcept -> const char*
{
    return d_ptr(impl_)->what.c_str();
}

auto Exception::StackTrace() const noexcept -> const char*
{
    return d_ptr(impl_)->stack_trace.c_str();
}

} // namespace hare
