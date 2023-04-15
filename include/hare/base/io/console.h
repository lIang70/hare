/**
 * @file hare/base/io/console.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with console.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_IO_CONSOLE_H_
#define _HARE_BASE_IO_CONSOLE_H_

#include <hare/base/io/event.h>
#include <hare/base/thread/thread.h>

#include <map>

namespace hare {
namespace io {

    class HARE_API console {
        ptr<event> console_event_ { nullptr };
        std::map<std::string, thread::task> handlers_ {};
        bool attached_ { false };

    public:
        using default_handle = std::function<void(const std::string& command_line)>;

        console();
        ~console();

        /**
         * @brief not thread-safe.
         **/
        void register_handle(std::string _handle_mask, thread::task _handle);
        void register_default_handle(default_handle _handle) const;
        auto attach(const ptr<cycle>& _cycle) -> bool;

    private:
        void process(const event::ptr& _event, uint8_t _events, const timestamp& _receive_time);
    };

} // namespace io
} // namespace hare

#endif // !_HARE_BASE_IO_CONSOLE_H_