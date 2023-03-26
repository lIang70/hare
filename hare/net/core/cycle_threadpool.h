#ifndef _HARE_NET_CORE_CYCLE_THREADPOOL_H_
#define _HARE_NET_CORE_CYCLE_THREADPOOL_H_

#include <hare/base/util/non_copyable.h>

#include <functional>
#include <memory>
#include <vector>
#include <string>

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
        CycleThreadPool(Cycle* base_cycle, std::string  reactor_type, std::string  name);
        ~CycleThreadPool();

        inline void setThreadNum(int num_threads) { num_threads_ = num_threads; }
        inline auto started() const -> bool { return started_; }
        auto name() const -> const std::string& { return name_; }

        auto start() -> bool;

        // valid after calling start()
        // round-robin
        auto getNextCycle() -> Cycle*;

        // with the same hash code, it will always return the same EventLoop
        auto getLoopForHash(size_t hash_code) -> Cycle*;

        auto getAllLoops() -> std::vector<Cycle*>;

    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_CORE_CYCLE_THREADPOOL_H_