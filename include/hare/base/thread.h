#ifndef _HARE_BASE_THREAD_H_
#define _HARE_BASE_THREAD_H_

#include <hare/base/detail/non_copyable.h>

#include <functional>
#include <string>

namespace hare {

class HARE_API Thread : public NonCopyable {
public:
    using Task = std::function<void()>;
    using Id = size_t;

private:
    class Data;
    Data* d_ { nullptr };

public:
    static int32_t threadCreated();

    explicit Thread(Task task, const std::string& name = std::string());
    ~Thread();

    const std::string& name() const;
    Id tid() const;
    bool started() const;

    bool isRunning();
    void start();
    bool join();
};

}

#endif // !_HARE_BASE_THREAD_H_