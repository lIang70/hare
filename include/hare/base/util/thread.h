/**
 * @file hare/base/util/thread.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with thread.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#ifndef _HARE_BASE_THREAD_H_
#define _HARE_BASE_THREAD_H_

#include <hare/base/error.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/base/util/non_copyable.h>

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace hare {

class HARE_API Thread : public NonCopyable {
public:
    using Task = std::function<void()>;
    using Id = size_t;

private:
    using UniqueThread = std::unique_ptr<std::thread>;
    using Handle = std::thread::native_handle_type;

    UniqueThread thread_ { nullptr };
    Thread::Task task_ {};
    std::string name_ {};
    Thread::Id thread_id_ {};
    bool started_ { false };
    util::CountDownLatch::Ptr count_down_latch_ { nullptr };

public:
    using Ptr = std::shared_ptr<Thread>;

    /**
     * @brief Get the number of threads created by class Thread.
     *
     */
    static auto threadCreated() -> int32_t;

    /**
     * @brief Construct a new Thread object.
     *
     * @param task The task of the thread.
     * @param name The name of thread. (Default: "HTHREAD-${thread_cnt}").
     */
    explicit Thread(Task task, std::string name = std::string());
    ~Thread();

    inline auto name() const -> const std::string& { return name_; }
    inline auto tid() const -> Id { return thread_id_; }
    inline auto started() const -> bool { return started_; }

    auto start() -> Error;
    auto join() -> Error;

private:
    void run();

};

} // namespace hare

#endif // !_HARE_BASE_THREAD_H_