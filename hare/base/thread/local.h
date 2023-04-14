#ifndef _HARE_BASE_LOCAL_H_
#define _HARE_BASE_LOCAL_H_

#include <hare/base/thread/thread.h>
#include <hare/hare-config.h>

namespace hare {
namespace io {
    class cycle;
}

namespace current_thread {

    struct local {
        thread::id tid { 0UL };
        std::string tid_str {};
        const char* tname { nullptr };
    };

    extern thread_local struct local t_data;

    extern void cache_thread_id();

    inline auto tid() -> thread::id
    {
        if (__builtin_expect(static_cast<int64_t>(t_data.tid == 0), 0) != 0) {
            cache_thread_id();
        }
        return t_data.tid;
    }

    inline auto tid_str() -> std::string
    {
        return t_data.tid_str;
    }

    inline auto thread_name() -> const char*
    {
        return t_data.tname;
    }

} // namespace current_thread
} // namespace hare

#endif // !_HARE_BASE_LOCAL_H_