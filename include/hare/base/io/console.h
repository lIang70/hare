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

HARE_CLASS_API
class HARE_API Console final : public NonCopyable {
  ::hare::detail::Impl* impl_{};

 public:
  using DefaultHandle = void (*)(const std::string& command_line);

  static auto Instance() -> Console&;

  ~Console();

  void RegisterDefaultHandle(DefaultHandle _default_handle);
  void RegisterHandle(std::string _handle_mask, Task _handle);
  auto Attach(Cycle* _cycle) -> bool;

  Console(Console&&) = delete;
  auto operator=(Console&&) -> Console = delete;

 private:
  Console();

  void Process(const ::hare::Ptr<Event>& _event, std::uint8_t _events,
               const Timestamp& _receive_time);
};

}  // namespace hare

#endif  // _HARE_BASE_IO_CONSOLE_H_
