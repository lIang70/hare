#include <hare/base/error.h>

#include <array>
#include <cerrno>

namespace hare {

namespace detail {

    static const auto description_length { HARE_ERRORS };
    static std::array<const char*, description_length> error_description {
        "Illegal error occurred, please contact the developer.", // HARE_ERROR_ILLEGAL
        "Success.", // HARE_ERROR_SUCCESS
        "The thread is already running.", // HARE_ERROR_THREAD_ALREADY_RUNNING
        "", // HARE_ERROR_THREAD_TASK_EMPTY
        "", // HARE_ERROR_FAILED_CREATE_THREAD
        "", // HARE_ERROR_JOIN_SAME_THREAD
        "", // HARE_ERROR_THREAD_EXITED
        "", // HARE_ERROR_TPOOL_ALREADY_RUNNING
        "", // HARE_ERROR_TPOOL_NOT_RUNNING
        "", // HARE_ERROR_TPOOL_THREAD_ZERO
    };

} // namespace detail

Error::Error(int32_t error_code)
    : error_code_(error_code)
    , system_code_(errno)
{
}

auto Error::description() const -> const char*
{
    return detail::error_description[error_code_];
}

} // namespace hare