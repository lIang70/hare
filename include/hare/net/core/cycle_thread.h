#ifndef _HARE_NET_CORE_CYCLE_THREAD_H_
#define _HARE_NET_CORE_CYCLE_THREAD_H_

#include <hare/base/thread.h>

#include <condition_variable>
#include <mutex>

namespace hare {
namespace core {

    class Cycle;
    class CycleThreadPrivate;
    class CycleThread : public Thread {
        CycleThreadPrivate* p_ { nullptr };

    public:
        explicit CycleThread(std::string  reactor_type, const std::string& name = "Cycle-Thread");
        ~CycleThread();

        Cycle* startCycle();

    protected:
        void run();
    };

} // namespace core
} // namespace hare

#endif // !_HARE_NET_CORE_CYCLE_THREAD_H_