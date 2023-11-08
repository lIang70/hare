#include <hare/base/exception.h>
#include <hare/base/io/buffer.h>
#include <hare/base/io/file.h>

#include "base/fwd_inl.h"
#include "base/io/reactor.h"

#ifndef H_OS_WIN
#include <sys/fcntl.h>

namespace hare {
namespace file_inner {

HARE_INLINE
util_socket_t OpenFile(const std::string& path, File::Mode mode) {
  int flags{0};
  if (CHECK_EVENT(mode, File::Mode::READ)) {
    SET_EVENT(flags, O_RDONLY);
  } else if (CHECK_EVENT(mode, File::Mode::WRITE)) {
    SET_EVENT(flags, O_WRONLY);
  } else if (CHECK_EVENT(mode, File::Mode::READWRITE)) {
    SET_EVENT(flags, O_RDWR);
  }
  if (CHECK_EVENT(mode, File::Mode::APPEND)) {
    SET_EVENT(flags, O_APPEND);
  }
  if (CHECK_EVENT(mode, File::Mode::CREATE)) {
    SET_EVENT(flags, O_CREAT);
  }
  if (CHECK_EVENT(mode, File::Mode::TRUNCATE)) {
    SET_EVENT(flags, O_TRUNC);
  }
  if (CHECK_EVENT(mode, File::Mode::EXCLUSIVE)) {
    SET_EVENT(flags, O_EXCL);
  }
  if (CHECK_EVENT(mode, File::Mode::SYNC)) {
    SET_EVENT(flags, O_SYNC);
  }
  if (CHECK_EVENT(mode, File::Mode::ASYNC)) {
    SET_EVENT(flags, O_ASYNC);
  }
  if (CHECK_EVENT(mode, File::Mode::NONBLOCK)) {
    SET_EVENT(flags, O_NONBLOCK);
  }

  util_socket_t fd = ::open(path.c_str(), flags, 0666);
  if (fd < 0) {
    HARE_INTERNAL_FATAL("Cannot open file {}.", path);
  }
  return fd;
}

}  // namespace file_inner

// clang-format off
HARE_IMPL_DEFAULT(File,
  File* file_;
  Buffer write_buffer{};

  explicit FileImpl(File* file) : file_(file) {}

  void HandleEvent(const Event*, std::uint8_t, const Timestamp&);
)
// clang-format on

void FileImpl::HandleEvent(const Event* event, std::uint8_t events,
                           const Timestamp& timestamp) {
  if (CHECK_EVENT(events, EVENT_READ)) {
  }
  if (CHECK_EVENT(events, EVENT_WRITE)) {
    write_buffer.Write(file_->fd(), 0);
  }
}

File::File(const std::string& path, Mode mode)
    : Event(file_inner::OpenFile(path, mode),
            std::bind(
                [this](const Event* event, std::uint8_t events,
                       const Timestamp& timestamp) {
                  IMPL->HandleEvent(event, events, timestamp);
                },
                std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3),
            EVENT_READ | EVENT_WRITE | EVENT_PERSIST, 0),
      impl_(new FileImpl{this}) {}

File::~File() { delete impl_; }

}  // namespace hare

#endif