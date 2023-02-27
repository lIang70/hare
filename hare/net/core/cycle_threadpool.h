#ifndef _HARE_NET_CORE_CYCLE_THREADPOOL_H_
#define _HARE_NET_CORE_CYCLE_THREADPOOL_H_

#include <hare/base/detail/non_copyable.h>

#include <functional>
#include <memory>
#include <vector>

namespace hare {
namespace core {

    class Cycle;
    class CycleThread;

    class CycleThreadPool : public NonCopyable {
        Cycle* base_cycle_ { nullptr };
        std::string reactor_type_ {};
        std::string name_ {};
        bool started_ { false };
        int num_threads_ { 0 };
        int next_ { 0 };
        std::vector<std::unique_ptr<CycleThread>> threads_ {};
        std::vector<Cycle*> cycles_ {};

    public:
        CycleThreadPool(Cycle* base_cycle, const std::string& reactor_type, const std::string& name);
        ~CycleThreadPool();

        inline void setThreadNum(int num_threads) { num_threads_ = num_threads; }
        inline bool started() const { return started_; }
        const std::string& name() const { return name_; }

        bool start();

        // valid after calling start()
        // round-robin
        Cycle* getNextCycle();

        // with the same hash code, it will always return the same EventLoop
        Cycle* getLoopForHash(size_t hash_code);

        std::vector<Cycle*> getAllLoops();

    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_CORE_CYCLE_THREADPOOL_H_