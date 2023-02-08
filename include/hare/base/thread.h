#ifndef _HARE_BASE_THREAD_H_
#define _HARE_BASE_THREAD_H_

#include <hare/base/detail/non_copyable.h>

#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace hare {

class ThreadData;
class Thread : public NonCopyable {
public:
    friend class ThreadData;

    using Task = std::function<void()>;
    using Id = size_t;

private:
    using UniqueThread = std::unique_ptr<std::thread>;

    UniqueThread thread_ { nullptr };
    Task task_ {};
    std::string name_ {};
    Id thread_id_ {};
    ThreadData* data_ { nullptr };
    bool started_ { false };

public:
    static int32_t threadCreated();

    explicit Thread(Task task, const std::string& name = std::string());
    ~Thread();

    bool isRunning();
    void start();
    bool join();

    inline Id tid() const { return thread_id_; }
    inline const std::string& name() const { return name_; }
    inline bool started() const { return started_; }
};

}

#endif // !_HARE_BASE_THREAD_H_