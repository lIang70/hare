#include <hare/net/timer.h>

namespace hare {
namespace net {

    Timer::Timer(int64_t ms_timeout, Thread::Task task, bool persist)
        : ms_timeout_(ms_timeout)
        , task_(std::move(task))
        , persist_(persist)
    {
    }

} // namespace net
} // namespace hare
