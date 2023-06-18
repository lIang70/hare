#include <hare/base/exception.h>

#include <hare/base/util/system_info.h>

namespace hare {

exception::exception(std::string what) noexcept
    : what_(std::move(what))
    , stack_(util::stack_trace(false))
{
}

} // namespace hare