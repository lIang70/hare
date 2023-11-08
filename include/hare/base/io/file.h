/**
 * @file hare/base/io/file.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with file.h
 * @version 0.2-beta
 * @date 2023-08-24
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_IO_FILE_H_
#define _HARE_BASE_IO_FILE_H_

#include <hare/base/io/event.h>

namespace hare {

HARE_CLASS_API
class HARE_API File : public Event {
  ::hare::detail::Impl* impl_{};

 public:
  enum Mode {
    READ = 0x01,
    WRITE = 0x02,
    READWRITE = READ | WRITE,
    APPEND = 0x04,
    CREATE = 0x08,
    TRUNCATE = 0x10,
    EXCLUSIVE = 0x20,
    SYNC = 0x40,
    ASYNC = 0x80,
    NONBLOCK = 0x100,
    DEFAULT = READWRITE | CREATE | TRUNCATE | EXCLUSIVE | NONBLOCK,
  };

  explicit File(const std::string& path, Mode mode = DEFAULT);
  ~File() override;
};

}  // namespace hare

#endif  // _HARE_BASE_IO_FILE_H_