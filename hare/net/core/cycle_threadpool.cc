#include "hare/net/core/cycle_threadpool.h"
#include "hare/net/core/cycle.h"
#include "hare/net/core/cycle_thread_p.h"
#include <hare/base/logging.h>

#include <utility>

namespace hare {
namespace core {

    CycleThreadPool::CycleThreadPool(Cycle* base_cycle, std::string reactor_type, std::string name)
        : base_cycle_(base_cycle)
        , reactor_type_(std::move(reactor_type))
        , name_(std::move(name))
    {
    }

    CycleThreadPool::~CycleThreadPool()
    {
        LOG_DEBUG() << "Thread-Pool[" << this << "] of cycles was deleted.";
        // todo
    }

    bool CycleThreadPool::start()
    {
        HARE_ASSERT(!started_, "");
        base_cycle_->assertInCycleThread();

        started_ = true;

        for (auto i = 0; i < num_threads_; ++i) {
            char buf[name_.size() + 32];
            snprintf(buf, sizeof(buf), "%s_%d", name_.c_str(), i);
            threads_.emplace_back(new CycleThread(reactor_type_, buf));
            cycles_.push_back(threads_[i]->startCycle());
        }

        return num_threads_ != 0;
    }

    Cycle* CycleThreadPool::getNextCycle()
    {
        base_cycle_->assertInCycleThread();
        HARE_ASSERT(started_, "");
        auto cycle = base_cycle_;

        if (!cycles_.empty()) {
            // round-robin
            cycle = cycles_[next_];
            ++next_;
            if (implicit_cast<size_t>(next_) >= cycles_.size()) {
                next_ = 0;
            }
        }
        return cycle;
    }

    Cycle* CycleThreadPool::getLoopForHash(size_t hash_code)
    {
        base_cycle_->assertInCycleThread();
        auto cycle = base_cycle_;

        if (!cycles_.empty()) {
            cycle = cycles_[hash_code % cycles_.size()];
        }

        return cycle;
    }

    std::vector<Cycle*> CycleThreadPool::getAllLoops()
    {
        base_cycle_->assertInCycleThread();
        HARE_ASSERT(started_, "");
        if (cycles_.empty()) {
            return { 1, base_cycle_ };
        } else {
            return cycles_;
        }
    }

} // namespace core
} // namespace hare