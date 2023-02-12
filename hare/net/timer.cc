#include <hare/net/timer.h>

namespace hare {

Timer::Timer(int64_t ms_timeout, Thread::Task task, bool persist)
    : ms_timeout_(ms_timeout)
    , task_(std::move(task))
    , persist_(persist)
{
}

void Timer::callback()
{
    if (task_)
        task_();
}

} // namespace hare
