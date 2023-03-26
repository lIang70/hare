#ifndef _HARE_NET_CORE_CYCLE_THREAD_H_
#define _HARE_NET_CORE_CYCLE_THREAD_H_

#include <hare/base/util/thread.h>

#include <condition_variable>
#include <mutex>

namespace hare {
namespace core {

    class Cycle;
    class HARE_API CycleThread : public Thread {
        Cycle* cycle_ { nullptr };
        std::string reactor_type_ {};
        std::condition_variable cv_ {};
        std::mutex mutex_ {};

    public:
        using Ptr = std::shared_ptr<CycleThread>;

        explicit CycleThread(std::string  reactor_type, std::string name = "CYCLE_THREAD");
        ~CycleThread();

        auto startCycle() -> Cycle*;

    protected:
        void run();
    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_CORE_CYCLE_THREAD_H_