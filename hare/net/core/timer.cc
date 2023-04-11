#include <hare/net/core/timer.h>

namespace hare {
namespace core {

    Timer::Timer(int64_t ms_timeout, Task task, bool persist)
        : ms_timeout_(ms_timeout)
        , task_(std::move(task))
        , persist_(persist)
    {
    }

} // namespace core
} // namespace hare
