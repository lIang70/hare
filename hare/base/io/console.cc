#include "hare/base/io/local.h"
#include "hare/base/io/reactor.h"
#include <hare/base/io/console.h>
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
            MSG_ERROR("unregistered command[{}], you can register \"default handle\" to console for handling all command.", command_line);
        }
    }

    auto console::instance() -> console&
    {
        static console s_console {};
        return s_console;
    }

    console::~console()
    {
        console_event_->tie(nullptr);
    }

    void console::register_default_handle(default_handle _handle)
    {
        default_ = std::move(_handle);
    }

    void console::register_handle(std::string _handle_mask, task _handle)
    {
        assert(!attached_);
        handlers_.emplace(_handle_mask, _handle);
    }

    auto console::attach(cycle* _cycle) -> bool
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

        console_event_->tie(console_event_);
        attached_ = true;
        return true;
    }

    console::console()
        : console_event_(new event(STDIN_FILENO,
            std::bind(&console::process, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            EVENT_READ | EVENT_PERSIST | EVENT_ET,
            0))
        , default_(detail::global_handle)
    {
    }

    void console::process(const event::ptr& _event, std::uint8_t _events, const timestamp& _receive_time)
    {
        assert(console_event_ == _event);
        if (!CHECK_EVENT(_events, EVENT_READ)) {
            MSG_ERROR("cannot check EVENT_READ.");
            return;
        }

        std::array<char, static_cast<std::size_t>(HARE_SMALL_BUFFER)> console_line {};

        auto len = ::read(_event->fd(), console_line.data(), static_cast<std::size_t>(HARE_SMALL_BUFFER));
        if (len < 0) {
            MSG_ERROR("cannot read from STDIN.");
            return;
        }
        std::string line(console_line.data(), len);
        while (line.back() == '\n') {
            line.pop_back();
        }

        MSG_TRACE("recv console input[{}] in {}.", line, _receive_time.to_fmt(true));

        auto iter = handlers_.find(line);
        if (iter != handlers_.end()) {
            iter->second();
        } else {
            default_(line);
        }
    }

} // namespace io
} // namespace hare