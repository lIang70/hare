/**
 * @file hare/base/io/timer.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with timer.h
 * @version 0.1-beta
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_IO_TIMER_H_
#define _HARE_BASE_IO_TIMER_H_

#include <hare/base/io/event.h>

namespace hare {

HARE_CLASS_API
class HARE_API Timer final : public Event {
  ::hare::detail::Impl* impl_{};

 public:
  Timer(Task _task, std::int64_t _timeval, bool is_persist = false);
  ~Timer() final;

  void Cancel();
};

}  // namespace hare

#endif  // _HARE_BASE_IO_TIMER_H_