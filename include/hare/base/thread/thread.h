/**
 * @file hare/base/thread/thread.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with thread.h
 * @version 0.1-beta
 * @date 2023-02-09
 * 
 * @copyright Copyright (c) 2023
 * 
 **/

#ifndef _HARE_BASE_THREAD_H_
#define _HARE_BASE_THREAD_H_

#include <hare/base/util/non_copyable.h>
#include <hare/base/util/count_down_latch.h>

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace hare {

class HARE_API thread : public non_copyable {
public:
    using task = std::function<void()>;
    using id = size_t;
    using ptr = ptr<thread>;

private:
    hare::ptr<std::thread> thread_ { nullptr };
    thread::task task_ {};
    std::string name_ {};
    thread::id thread_id_ {};
    bool started_ { false };
    util::count_down_latch::ptr count_down_latch_ { nullptr };

public:
    /**
     * @brief Get the number of threads created by class Thread.
     * 
     **/
    static auto thread_created() -> size_t;

    /**
     * @brief Construct a new Thread object.
     * 
     * @param _task Task of the thread.
     * @param _name name of thread. (Default: "HTHREAD-${thread_cnt}").
     **/
    explicit thread(task _task, std::string _name = std::string());
    ~thread();

    inline auto name() const -> const std::string& { return name_; }
    inline auto tid() const -> thread::id { return thread_id_; }
    inline auto started() const -> bool { return started_; }

    void start();
    auto join() -> bool;

private:
    void run();

};

} // namespace hare

#endif // !_HARE_BASE_THREAD_H_