#ifndef _HARE_BASE_LOCAL_H_
#define _HARE_BASE_LOCAL_H_

#include <cstdio>
#include <hare/base/util/thread.h>
#include <hare/hare-config.h>

namespace hare {

struct ThreadLocal {
    Thread::Id tid_ { 0UL };
    std::string tid_string_ {};
    const char* thread_name_ { nullptr };
};

extern thread_local struct ThreadLocal t_data;

namespace current_thread {

    extern void cacheThreadId();

    extern inline auto tid() -> Thread::Id
    {
        if (__builtin_expect(t_data.tid_ == 0, 0) != 0) {
            cacheThreadId();
        }
        return t_data.tid_;
    }

    inline auto tidString() -> std::string
    {
        return t_data.tid_string_;
    }

    inline auto threadName() -> const char*
    {
        return t_data.thread_name_;
    }

    extern auto isMainThread() -> bool;
    extern auto stackTrace(bool demangle) -> std::string;

}
}

#endif // !_HARE_BASE_LOCAL_H_