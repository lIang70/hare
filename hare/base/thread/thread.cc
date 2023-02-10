#include "hare/base/thread/local.h"
#include "hare/base/util/count_down_latch.h"
#include <hare/base/exception.h>
#include <hare/base/logging.h>
#include <hare/base/system_check.h>

#include <atomic>

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

class ThreadData {
public:
    using Handle = std::thread::native_handle_type;

    Thread* thread_ { nullptr };
    util::CountDownLatch* count_down_latch_ { nullptr };

    ThreadData(Thread* thread)
        : thread_(thread)
    {
    }

    void run()
    {
        thread_->thread_id_ = current_thread::tid();
        count_down_latch_->countDown();

        t_data.thread_name_ = thread_->name_.empty() ? "hareThread" : thread_->name_.c_str();

        // For debug
#ifdef H_OS_WIN32
        setCurrentThreadName(t_data.thread_name_);
#elif defined(H_OS_LINUX)
        ::prctl(PR_SET_NAME, t_data.thread_name_);
#endif

        try {
            thread_->task_();
            t_data.thread_name_ = "finished";
            thread_->thread_id_ = 0;
            thread_->started_ = false;
        } catch (const Exception& e) {
            t_data.thread_name_ = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", thread_->name_.c_str());
            fprintf(stderr, "reason: %s\n", e.what());
            fprintf(stderr, "stack trace: %s\n", e.stackTrace());
            std::abort();
        } catch (const std::exception& e) {
            t_data.thread_name_ = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", thread_->name_.c_str());
            fprintf(stderr, "reason: %s\n", e.what());
            std::abort();
        } catch (...) {
            t_data.thread_name_ = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", thread_->name_.c_str());
            throw;
        };
    }
};

static std::atomic_int32_t num_of_thread_ { 0 };

static void setDefaultName(std::string& name)
{
    auto id = num_of_thread_.fetch_add(1, std::memory_order_acquire);
    if (name.empty()) {
        name = "Thread-" + std::to_string(id);
    }
}

int32_t Thread::threadCreated()
{
    return num_of_thread_.load(std::memory_order_relaxed);
}

Thread::Thread(Task task, const std::string& name)
    : task_(std::move(task))
    , name_(name)
    , data_(new ThreadData(this))
{
    setDefaultName(name_);
}

Thread::~Thread()
{
    if (thread_ && thread_->joinable())
        thread_->join();

    if (data_)
        delete data_;
}

bool Thread::isRunning()
{
    return started_;
}

void Thread::start()
{
    HARE_ASSERT(!started_, "Thread is stared.");
    started_ = true;

    try {
        if (data_->count_down_latch_) {
            delete data_->count_down_latch_;
        }
        data_->count_down_latch_ = new util::CountDownLatch(1);
        thread_ = UniqueThread(new std::thread(&ThreadData::run, data_));
        data_->count_down_latch_->await();

    } catch (const Exception& e) {
        t_data.thread_name_ = "crashed";
        fprintf(stderr, "exception caught in tread created\n");
        fprintf(stderr, "reason: %s\n", e.what());
        fprintf(stderr, "stack trace: %s\n", e.stackTrace());
        std::abort();
    } catch (const std::exception& e) {
        t_data.thread_name_ = "crashed";
        fprintf(stderr, "Fail to create thread! Detail:\n %s", e.what());
        std::abort();
    }
}

bool Thread::join()
{
    if (current_thread::tid() == tid())
        return false;

    if (thread_ && thread_->joinable()) {
        thread_->join();
        return true;
    }

    return false;
}

}