#ifndef _HARE_BASE_IO_LOCAL_H_
#define _HARE_BASE_IO_LOCAL_H_

#include <hare/base/io/event.h>
#include <hare/hare-config.h>

#include <sstream>
#include <thread>

namespace hare {
namespace io {
    class cycle;

    namespace current_thread {

        using TID = struct thread_io_data {
            // io
            io::cycle* cycle { nullptr };

            // thread
            std::uint64_t tid {};

            thread_io_data()
            {
                std::ostringstream oss {};
                oss << std::this_thread::get_id();
                tid = std::stoull(oss.str());
            }

            thread_io_data(const thread_io_data&) = delete;
            thread_io_data(thread_io_data&&) noexcept = delete;

            thread_io_data& operator=(const thread_io_data&) = delete;
            thread_io_data& operator=(thread_io_data&&) = delete;
        };

        HARE_INLINE
        auto get_tds() -> TID&
        {
            static thread_local struct thread_io_data t { };
            return t;
        }

    } // namespace current_thread

} // namespace io
} // namespace hare

#endif // _HARE_BASE_IO_LOCAL_H_