#include "hare/net/core/cycle.h"
#include <hare/net/core/cycle_thread.h>
#include <hare/base/logging.h>

#include <condition_variable>
#include <mutex>

namespace hare {
namespace core {

    class CycleThreadPrivate {
    public:
        Cycle* cycle_ { nullptr };
        std::string reactor_type_ {};
        std::condition_variable cv_ {};
        std::mutex mutex_ {};

    };

    CycleThread::CycleThread(std::string reactor_type, const std::string& name)
        : Thread(std::bind(&CycleThread::run, this), name)
        , p_(new CycleThreadPrivate)
    {
        p_->reactor_type_ = std::move(reactor_type);
    }

    CycleThread::~CycleThread()
    {
        if (!p_->cycle_) {
            // still a tiny chance to call destructed object, if run exits just now.
            // but when EventLoopThread destructs, usually programming is exiting anyway.
            p_->cycle_->exit();
            join();
        }
        delete p_;
    }

    Cycle* CycleThread::startCycle()
    {
        HARE_ASSERT(!started(), "");
        start();

        Cycle* cycle { nullptr };
        {
            std::unique_lock<std::mutex> locker(p_->mutex_);
            while (!p_->cycle_) {
                p_->cv_.wait(locker);
            }
            cycle = p_->cycle_;
        }

        return cycle;
    }

    void CycleThread::run()
    {
        Cycle cycle(p_->reactor_type_);

        {
            std::unique_lock<std::mutex> locker(p_->mutex_);
            p_->cycle_ = &cycle;
            p_->cv_.notify_one();
        }

        LOG_TRACE() << "Thread of cycle is running...";

        cycle.loop();

        LOG_TRACE() << "Cycle of thread is exited...";

        std::unique_lock<std::mutex> locker(p_->mutex_);
        p_->cycle_ = nullptr;
    }

} // namespace core
} // namespace hare