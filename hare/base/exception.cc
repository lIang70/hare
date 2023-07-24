#include <hare/base/exception.h>
#include <hare/base/util/system.h>

namespace hare {

namespace detail {

    HARE_IMPL_DEFAULT(exception,
        std::string what_ {};
        std::string stack_ {};
    )

    static auto thread_exception() -> exception_impl* {
        static thread_local exception_impl t_impl {};
        return &t_impl;
    }

} // namespace detail

exception::exception(std::string what) noexcept
    : impl_(detail::thread_exception())
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
