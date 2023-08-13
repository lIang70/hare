#include "hare/base/fwd-inl.h"
#include "hare/base/io/reactor.h"
#include <hare/base/io/console.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/hare-config.h>

#include <map>

#if HARE__HAVE_UNISTD_H
#include <unistd.h>
#endif // HARE__HAVE_UNISTD_H

#if HARE__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif // HARE__HAVE_SYS_SOCKET_H

#if defined(H_OS_WIN32)
#define STDIN_FILENO 0
#endif // H_OS_WIN32

namespace hare {
namespace io {

    namespace detail {
        static void GlobalConsoleHandle(const std::string& _command_line)
        {
            HARE_INTERNAL_ERROR("unregistered command[{}], you can register \"default handle\" to console for handling all command.", _command_line);
        }
    }

    HARE_IMPL_DEFAULT(
        Console,
        Ptr<Event> console_event { nullptr };
        std::map<std::string, Task> handlers {};
        Console::DefaultHandle default_handle {};
        bool attached { false };
    )

    auto Console::Instance() -> Console&
    {
        static Console static_console {};
        return static_console;
    }

    Console::~Console()
    {
        d_ptr(impl_)->console_event->Tie(nullptr);
        delete impl_;
    }

    void Console::RegisterDefaultHandle(DefaultHandle _default_handle)
    {
        d_ptr(impl_)->default_handle = std::move(_default_handle);
    }

    void Console::RegisterHandle(std::string _handle_mask, Task _handle)
    {
        assert(!d_ptr(impl_)->attached);
        d_ptr(impl_)->handlers.emplace(_handle_mask, _handle);
    }

    auto Console::Attach(Cycle* _cycle) -> bool
    {
        if (!_cycle) {
            return false;
        }
        if (_cycle->is_running()) {
            auto in_cycle_thread = _cycle->InCycleThread();
            auto cdl = std::make_shared<util::CountDownLatch>(1);
            _cycle->RunInCycle([=] {
                _cycle->EventUpdate(d_ptr(impl_)->console_event);
                cdl->CountDown();
            });

            if (!in_cycle_thread) {
                cdl->Await();
            }

        } else {
            _cycle->EventUpdate(d_ptr(impl_)->console_event);
        }

        d_ptr(impl_)->console_event->Tie(d_ptr(impl_)->console_event);
        d_ptr(impl_)->attached = true;
        return true;
    }

    Console::Console()
        : impl_(new ConsoleImpl)
    {
        d_ptr(impl_)->console_event.reset(new Event(STDIN_FILENO,
            std::bind(&Console::Process, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            EVENT_READ | EVENT_PERSIST | EVENT_ET,
            0));
        d_ptr(impl_)->default_handle = detail::GlobalConsoleHandle;
    }

    void Console::Process(const Ptr<Event>& _event, std::uint8_t _events, const Timestamp& _receive_time)
    {
        assert(d_ptr(impl_)->console_event == _event);
        if (CHECK_EVENT(_events, EVENT_READ) == 0) {
            HARE_INTERNAL_ERROR("cannot check EVENT_READ.");
            return;
        }

        std::array<char, HARE_SMALL_BUFFER> console_line {};

#if defined(H_OS_WIN32)
        auto len = ::_read((int)_event->fd(), console_line.data(), HARE_SMALL_BUFFER);
#else
        auto len = ::read(_event->fd(), console_line.data(), HARE_SMALL_BUFFER);
#endif // H_OS_WIN32

        if (len < 0) {
            HARE_INTERNAL_ERROR("cannot read from STDIN.");
            return;
        }
        std::string line(console_line.data(), len);
        while (line.back() == '\n') {
            line.pop_back();
        }

        HARE_INTERNAL_TRACE("recv console input[{}] in {}.", line, _receive_time.ToFmt(true));

        auto iter = d_ptr(impl_)->handlers.find(line);
        if (iter != d_ptr(impl_)->handlers.end()) {
            iter->second();
        } else {
            d_ptr(impl_)->default_handle(line);
        }
    }

} // namespace io
} // namespace hare
