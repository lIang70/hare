#include <hare/base/thread/thread.h>

#include "hare/base/thread/local.h"
#include <hare/base/logging.h>
#include <hare/base/util/system_info.h>
#include <hare/base/exception.h>

namespace hare {

namespace detail {
    static std::atomic<size_t> s_num_of_thread { 0 };

    static void set_default_name(std::string& name)
    {
        auto tid = s_num_of_thread.fetch_add(1, std::memory_order_acquire);
        if (name.empty()) {
            name = "HTHREAD-" + std::to_string(tid);
        }
    }

} // namespace detail

auto thread::thread_created() -> size_t
{
    return detail::s_num_of_thread.load(std::memory_order_relaxed);
}

thread::thread(task task, std::string name)
    : task_(std::move(task))
    , name_(std::move(name))
{
    detail::set_default_name(name_);
}

thread::~thread()
{
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

void thread::start()
{
    HARE_ASSERT(!started_, "thread is stared.");
    started_ = true;

    try {
        count_down_latch_.reset(new util::count_down_latch(1));
        thread_ = std::make_shared<std::thread>(&thread::run, this);
        count_down_latch_->await();
    } catch (const std::exception& e) {
        current_thread::get_tds().tname = "crashed";
        auto ret = ::fprintf(stderr, "fail to create thread! Detail:\n %s", e.what());
        H_UNUSED(ret);
        std::abort();
    } catch (...) {
        auto ret = ::fprintf(stderr, "unknown exception caught in thread %s\n", name_.c_str());
        H_UNUSED(ret);
        throw; // rethrow
    }
}

auto thread::join() -> bool
{
    if (current_thread::tid() == tid()) {
        SYS_ERROR() << "cannot join in the same thread.";
        return false;
    }

    if (started_ && thread_ && thread_->joinable()) {
        thread_->join();
        return true;
    }

    return false;
}

void thread::run()
{
    thread_id_ = current_thread::tid();

    if (count_down_latch_) {
        count_down_latch_->count_down();
    }

    current_thread::get_tds().tname = name_.empty() ? "HARE_THREAD" : name_.c_str();

    // For debug
    util::set_thread_name(name_.c_str());

    try {
        task_();
        current_thread::get_tds().tname = "finished";
        thread_id_ = 0;
        started_ = false;
    } catch (const exception& e) {
        current_thread::get_tds().tname = "crashed";
        auto ret = ::fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        ret = ::fprintf(stderr, "reason: %s\n", e.what());
        ret = ::fprintf(stderr, "stack trace: %s\n", e.stack_trace());
        H_UNUSED(ret);
        std::abort();
    } catch (const std::exception& e) {
        current_thread::get_tds().tname = "crashed";
        auto ret = ::fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        ret = ::fprintf(stderr, "reason: %s\n", e.what());
        H_UNUSED(ret);
        std::abort();
    } catch (...) {
        current_thread::get_tds().tname = "crashed";
        auto ret = ::fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        H_UNUSED(ret);
        throw;
    }
}

} // namespace hare