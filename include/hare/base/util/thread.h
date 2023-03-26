#ifndef _HARE_BASE_THREAD_H_
#define _HARE_BASE_THREAD_H_

#include <hare/base/util/non_copyable.h>
#include <hare/base/util/count_down_latch.h>

#include <atomic>
#include <thread>
#include <functional>
#include <string>

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

    static auto threadCreated() -> int32_t;

    explicit Thread(Task task, std::string name = std::string());
    ~Thread();

    inline auto name() const -> const std::string& { return name_; }
    inline auto tid() const -> Id { return thread_id_; }
    inline auto started() const -> bool { return started_; }

    void start();
    auto join() -> bool;

private:
    void run();

};

} // namespace hare

#endif // !_HARE_BASE_THREAD_H_