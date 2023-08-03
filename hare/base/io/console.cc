#include "hare/base/fwd-inl.h"
#include "hare/base/io/reactor.h"
#include <hare/base/io/console.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/hare-config.h>

#include <map>

#if HARE__HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HARE__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if defined(H_OS_WIN32)
#define STDIN_FILENO 0
#endif

namespace hare {
namespace io {

    namespace detail {
        static void global_handle(const std::string& command_line)
        {
            MSG_ERROR("unregistered command[{}], you can register \"default handle\" to console for handling all command.", command_line);
        }
    }

    HARE_IMPL_DEFAULT(console,
        ptr<event> console_event_ { nullptr };
        std::map<std::string, task> handlers_ {};
        console::default_handle default_ {};
        bool attached_ { false };
    )

    auto console::instance() -> console&
    {
        static console s_console {};
        return s_console;
    }

    console::~console()
    {
        d_ptr(impl_)->console_event_->tie(nullptr);
        delete impl_;
    }

    void console::register_default_handle(default_handle _handle)
    {
        d_ptr(impl_)->default_ = std::move(_handle);
    }

    void console::register_handle(std::string _handle_mask, task _handle)
    {
        assert(!d_ptr(impl_)->attached_);
        d_ptr(impl_)->handlers_.emplace(_handle_mask, _handle);
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
                _cycle->event_update(d_ptr(impl_)->console_event_);
                cdl->count_down();
            });

            if (!in_cycle_thread) {
                cdl->await();
            }

        } else {
            _cycle->event_update(d_ptr(impl_)->console_event_);
        }

        d_ptr(impl_)->console_event_->tie(d_ptr(impl_)->console_event_);
        d_ptr(impl_)->attached_ = true;
        return true;
    }

    console::console()
        : impl_(new console_impl)
    {
        d_ptr(impl_)->console_event_.reset(new event(STDIN_FILENO,
            std::bind(&console::process, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            EVENT_READ | EVENT_PERSIST | EVENT_ET,
            0));
        d_ptr(impl_)->default_ = detail::global_handle;
    }

    void console::process(const event::ptr& _event, std::uint8_t _events, const timestamp& _receive_time)
    {
        assert(d_ptr(impl_)->console_event_ == _event);
        if (CHECK_EVENT(_events, EVENT_READ) == 0) {
            MSG_ERROR("cannot check EVENT_READ.");
            return;
        }

        std::array<char, HARE_SMALL_BUFFER> console_line {};

#if defined(H_OS_WIN32)
        auto len = ::_read((int)_event->fd(), console_line.data(), HARE_SMALL_BUFFER);
#else
        auto len = ::read(_event->fd(), console_line.data(), HARE_SMALL_BUFFER);
#endif

        if (len < 0) {
            MSG_ERROR("cannot read from STDIN.");
            return;
        }
        std::string line(console_line.data(), len);
        while (line.back() == '\n') {
            line.pop_back();
        }

        MSG_TRACE("recv console input[{}] in {}.", line, _receive_time.to_fmt(true));

        auto iter = d_ptr(impl_)->handlers_.find(line);
        if (iter != d_ptr(impl_)->handlers_.end()) {
            iter->second();
        } else {
            d_ptr(impl_)->default_(line);
        }
    }

} // namespace io
} // namespace hare
