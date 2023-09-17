#include <hare/base/exception.h>
#include <hare/base/system.h>

#include "base/fwd-inl.h"

namespace hare {

namespace detail {

HARE_IMPL_DEFAULT(Exception, std::string what{}; std::string stack_trace{};)

static auto ThreadException() -> ExceptionImpl* {
  static thread_local ExceptionImpl thread_exception_impl{};
  return &thread_exception_impl;
}

}  // namespace detail

Exception::Exception(std::string what) noexcept
    : impl_(detail::ThreadException()) {
  IMPL->what = std::move(what);
  IMPL->stack_trace = ::hare::StackTrace(false);
}

Exception::~Exception() noexcept { delete impl_; }

auto Exception::what() const noexcept -> const char* {
  return IMPL->what.c_str();
}

auto Exception::StackTrace() const noexcept -> const char* {
  return IMPL->stack_trace.c_str();
}

}  // namespace hare
