#include <hare/base/io/console.h>
#include <hare/base/util/count_down_latch.h>
#include <hare/hare-config.h>

#include "base/fwd_inl.h"
#include "base/io/operation_inl.h"
#include "base/io/reactor.h"

#define STDIN_FILENO 0

namespace hare {

namespace io_inner {
static void GlobalConsoleHandle(const std::string& _command_line) {
  HARE_INTERNAL_ERROR(
      "unregistered command[{}], you can register \"default handle\" to "
      "console for handling all command.",
      _command_line);
}
}  // namespace io_inner

// clang-format off
HARE_IMPL_DEFAULT(Console,
  ::hare::Ptr<Event> console_event{nullptr};
  std::map<std::string, Task> handlers{};
  AtomicHook<Console::DefaultHandle> default_handle{
    io_inner::GlobalConsoleHandle};
  bool attached{false};

  void Process(const Event* _event, std::uint8_t _events, const Timestamp& _receive_time);
)
// clang-format on

void ConsoleImpl::Process(const Event* _event, std::uint8_t _events,
                          const Timestamp& _receive_time) {
  HARE_ASSERT(console_event.get() == _event);
  if (CHECK_EVENT(_events, EVENT_READ) == 0) {
    HARE_INTERNAL_ERROR("cannot check EVENT_READ.");
    return;
  }

  std::array<char, HARE_SMALL_BUFFER> console_line{};

  auto len =
      ::hare::io::Read(_event->fd(), console_line.data(), HARE_SMALL_BUFFER);

  if (len < 0) {
    HARE_INTERNAL_ERROR("cannot read from STDIN.");
    return;
  }
  std::string line(console_line.data(), len);
  while (line.back() == '\n') {
    line.pop_back();
  }

  HARE_INTERNAL_TRACE("recv console input[{}] in {}.", line,
                      _receive_time.ToFmt(true));

  auto iter = handlers.find(line);
  if (iter != handlers.end()) {
    iter->second();
  } else {
    default_handle(line);
  }
}

auto Console::Instance() -> Console& {
  static Console static_console{};
  return static_console;
}

Console::~Console() {
  IMPL->console_event->Tie(nullptr);
  delete impl_;
}

void Console::RegisterDefaultHandle(DefaultHandle _default_handle) {
  IMPL->default_handle.Store(_default_handle);
}

void Console::RegisterHandle(std::string _handle_mask, Task _handle) {
  HARE_ASSERT(!IMPL->attached);
  IMPL->handlers.emplace(_handle_mask, _handle);
}

auto Console::Attach(Cycle* _cycle) -> bool {
  if (!_cycle) {
    return false;
  }
  _cycle->EventUpdate(IMPL->console_event.get());

  IMPL->console_event->Tie(IMPL->console_event);
  IMPL->attached = true;
  return true;
}

Console::Console() : impl_(new ConsoleImpl) {
  IMPL->console_event.reset(
      new Event(STDIN_FILENO,
                std::bind(&ConsoleImpl::Process, IMPL, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3),
                EVENT_READ | EVENT_PERSIST | EVENT_ET, 0));
  IMPL->default_handle.Store(io_inner::GlobalConsoleHandle);
}

}  // namespace hare
