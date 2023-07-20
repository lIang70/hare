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

namespace hare {
namespace io {

    HARE_CLASS_API
    class HARE_API console : public util::non_copyable {
        hare::detail::impl* impl_ {};

    public:
        using default_handle = std::function<void(const std::string& command_line)>;

        static auto instance() -> console&;

        ~console();

        /**
         * @brief not thread-safe.
         **/
        void register_default_handle(default_handle _handle);
        void register_handle(std::string _handle_mask, task _handle);
        auto attach(cycle* _cycle) -> bool;

        console(console&&) = delete;
        auto operator=(console&&) -> console = delete;

    private:
        console();

        void process(const event::ptr& _event, uint8_t _events, const timestamp& _receive_time);
    };

} // namespace io
} // namespace hare

#endif // _HARE_BASE_IO_CONSOLE_H_
