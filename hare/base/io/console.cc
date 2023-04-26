#include <cstdio>
#include <hare/base/io/console.h>

#include "hare/base/io/reactor.h"
#include <hare/base/logging.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/hare-config.h>

#if HARE__HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HARE__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

namespace hare {
namespace io {

    namespace detail {
        
        static void global_handle(const std::string& command_line)
        {
            SYS_ERROR() << "unregistered command[" << command_line << "], you can register \"default handle\" to console for handling all command.";
        }

        static console::default_handle s_command_handle = global_handle;
    }

    console::console()
        : console_event_(new event(STDIN_FILENO,
            std::bind(&console::process, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            EVENT_READ | EVENT_PERSIST,
            0))
    {
    }

    console::~console()
    {
        console_event_->deactivate();
    }

    void console::register_handle(std::string _handle_mask, task _handle)
    {
        HARE_ASSERT(!attached_, "you need add handle to console before attached.");
        handlers_.emplace(_handle_mask, _handle);
    }

    void console::register_default_handle(default_handle _handle) const
    {
        HARE_ASSERT(!attached_, "you need add handle to console before attached.");
        detail::s_command_handle = std::move(_handle);
    }

    auto console::attach(const ptr<cycle>& _cycle) -> bool
    {
        if (!_cycle) {
            return false;
        }
        if (_cycle->is_running()) {
            auto in_cycle_thread = _cycle->in_cycle_thread();
            auto cdl = std::make_shared<util::count_down_latch>(1);
            _cycle->run_in_cycle([=] {
                _cycle->event_update(console_event_);
                cdl->count_down();
            });

            if (!in_cycle_thread) {
                cdl->await();
            }

        } else {
            _cycle->event_update(console_event_);
        }
        attached_ = true;
        return true;
    }

    void console::process(const event::ptr& _event, uint8_t _events, const timestamp& _receive_time)
    {
        HARE_ASSERT(console_event_ == _event, "error event.");
        if (!CHECK_EVENT(_events, EVENT_READ)) {
            SYS_ERROR() << "cannot check EVENT_READ.";
            return ;
        }

        std::array<char, static_cast<size_t>(HARE_SMALL_BUFFER)> console_line {};

        auto len = ::read(_event->fd(), console_line.data(), static_cast<size_t>(HARE_SMALL_BUFFER));
        if (len < 0) {
            SYS_ERROR() << "cannot read from STDIN.";
            return;
        }
        std::string line(console_line.data(), len);
        while (line.back() == '\n') {
            line.pop_back();
        }
        LOG_TRACE() << "recv console input[" << line << "] in " << _receive_time.to_fmt(true);

        auto iter = handlers_.find(line);
        if (iter != handlers_.end()) {
            iter->second();
        } else {
            detail::s_command_handle(line);
        }
    }

} // namespace io
} // namespace hare