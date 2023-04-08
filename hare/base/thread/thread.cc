#include "hare/base/error.h"
#include "hare/base/thread/local.h"
#include <hare/base/exception.h>
#include <hare/base/logging.h>
#include <hare/base/util/system_check.h>

#ifdef H_OS_WIN32
#include <Windows.h>

typedef struct THREADNAME_INFO {
    DWORD dwType { 0x1000 }; // must be 0x1000
    LPCSTR szName; // pointer to name (in user addr space)
    DWORD dwThreadID { -1 }; // thread ID (-1=caller thread)
    DWORD dwFlags { 0 }; // reserved for future use, must be zero
} THREADNAME_INFO;

void setCurrentThreadName(LPCSTR ThreadName)
{
    THREADNAME_INFO info;
    info.szName = ThreadName;

    __try {
        RaiseException(0x406D1388, 0, sizeof(info) / sizeof(DWORD), (DWORD*)&info);
    } __except (EXCEPTION_CONTINUE_EXECUTION) {
    }
}

#elif defined(H_OS_LINUX)
#include <sys/prctl.h>
#endif

namespace hare {

static std::atomic<int32_t> num_of_thread_ { 0 };

static void setDefaultName(std::string& name)
{
    auto tid = num_of_thread_.fetch_add(1, std::memory_order_acquire);
    if (name.empty()) {
        name = "HTHREAD-" + std::to_string(tid);
    }
}

auto Thread::threadCreated() -> int32_t
{
    return num_of_thread_.load(std::memory_order_relaxed);
}

Thread::Thread(Task task, std::string name)
    : task_(std::move(task))
    , name_(std::move(name))
{
    setDefaultName(name_);
}

Thread::~Thread()
{
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

auto Thread::start() -> Error
{
    if (started_) {
        return Error(HARE_ERROR_THREAD_ALREADY_RUNNING);
    }

    if (!task_) {
        return Error(HARE_ERROR_THREAD_TASK_EMPTY);
    }

    try {
        count_down_latch_.reset(new util::CountDownLatch(1));
        thread_ = UniqueThread(new std::thread(&Thread::run, this));
        count_down_latch_->await();
        started_ = true;
        return Error(HARE_ERROR_SUCCESS);
    } catch (const std::exception& e) {
        t_data.thread_name_ = "crashed";
        fprintf(stderr, "Fail to create thread! Detail:\n %s", e.what());
    } catch (...) {
        fprintf(stderr, "unknown exception caught in create thread %s\n", name_.c_str());
    }

    return Error(HARE_ERROR_FAILED_CREATE_THREAD);
}

auto Thread::join() -> Error
{
    if (current_thread::tid() == tid()) {
        SYS_ERROR() << "Cannot join in the same thread.";
        return Error(HARE_ERROR_JOIN_SAME_THREAD);
    }

    if (thread_ && thread_->joinable()) {
        thread_->join();
        return Error(HARE_ERROR_SUCCESS);
    }

    return Error(HARE_ERROR_THREAD_EXITED);
}

void Thread::run()
{
    thread_id_ = current_thread::tid();

    if (count_down_latch_) {
        count_down_latch_->countDown();
    }

    t_data.thread_name_ = name_.empty() ? "HareThread" : name_.c_str();

    // For debug
#ifdef H_OS_WIN32
    setCurrentThreadName(t_data.thread_name_);
#elif defined(H_OS_LINUX)
    ::prctl(PR_SET_NAME, t_data.thread_name_);
#endif

    try {
        task_();
        t_data.thread_name_ = "finished";
        thread_id_ = 0;
        started_ = false;
    } catch (const Exception& e) {
        t_data.thread_name_ = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", e.what());
        fprintf(stderr, "stack trace: %s\n", e.stackTrace());
        std::abort();
    } catch (const std::exception& e) {
        t_data.thread_name_ = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", e.what());
        std::abort();
    } catch (...) {
        t_data.thread_name_ = "crashed";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        throw;
    }
}

} // namespace hare