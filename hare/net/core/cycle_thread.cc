#include "hare/net/core/cycle.h"
#include <hare/net/core/cycle_thread.h>
#include <hare/base/logging.h>

#include <utility>

namespace hare {
namespace core {

    CycleThread::CycleThread(std::string reactor_type, std::string name)
        : Thread(std::bind(&CycleThread::run, this), std::move(name))
        , reactor_type_(std::move(reactor_type))
    {
    }

    CycleThread::~CycleThread()
    {
        if (cycle_ == nullptr) {
            // still a tiny chance to call destructed object, if run exits just now.
            // but when EventLoopThread destructs, usually programming is exiting anyway.
            cycle_->exit();
            join();
        }
    }

    auto CycleThread::startCycle() -> Cycle*
    {
        HARE_ASSERT(!started(), "");
        start();

        Cycle* cycle { nullptr };
        {
            std::unique_lock<std::mutex> locker(mutex_);
            while (cycle_ == nullptr) {
                cv_.wait(locker);
            }
            cycle = cycle_;
        }

        return cycle;
    }

    void CycleThread::run()
    {
        Cycle cycle(reactor_type_);

        {
            std::unique_lock<std::mutex> locker(mutex_);
            cycle_ = &cycle;
            cv_.notify_one();
        }

        LOG_TRACE() << "Thread of cycle is running...";

        cycle.loop();

        LOG_TRACE() << "Cycle of thread is exited...";

        std::unique_lock<std::mutex> locker(mutex_);
        cycle_ = nullptr;
    }

} // namespace core
} // namespace hare