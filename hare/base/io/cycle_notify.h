#ifndef _HARE_BASE_IO_CYCLE_NOTIFY_H_
#define _HARE_BASE_IO_CYCLE_NOTIFY_H_

#include <hare/base/io/event.h>
#include <hare/hare-config.h>

namespace hare {

class CycleNotify : public Event {
#ifndef HARE__HAVE_EVENTFD
  util_socket_t write_fd_{};
#endif

 public:
  CycleNotify();
  ~CycleNotify() override;

  void SendNotify() const;

 protected:
  void EventCallback(const Event* _event, std::uint8_t _events,
                     const Timestamp& _receive_time);
};

}  // namespace hare

#endif  // _HARE_BASE_IO_CYCLE_NOTIFY_H_