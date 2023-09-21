#include <hare/base/io/file.h>

#include "base/fwd_inl.h"

#ifndef H_OS_WIN
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace hare {
namespace io_inner {

auto Open(std::FILE** _fp, const filename_t& _filename, const filename_t& _mode)
    -> bool {
  *_fp =
      ::fopen(FilenameToStr(_filename).c_str(), FilenameToStr(_mode).c_str());
  return *_fp != nullptr;
}

auto Exists(const filename_t& _filepath) -> bool {
  struct stat buffer {};
  return (::stat(FilenameToStr(_filepath).c_str(), &buffer) == 0);
}

auto Remove(const filename_t& _filepath) -> bool {
  return std::remove(FilenameToStr(_filepath).c_str()) == 0;
}

auto Size(std::FILE* _fp) -> std::size_t {
  if (_fp == nullptr) {
    return 0;
  }
  auto fd = ::fileno(_fp);
#ifdef __USE_LARGEFILE64
#define STAT struct stat64
#define FSTAT ::fstat64
#else
#define STAT struct stat
#define FSTAT ::fstat
#endif
  STAT st{};
  if (FSTAT(fd, &st) == 0) {
    return static_cast<std::size_t>(st.st_size);
  } else {
    return 0;
  }
#undef STAT
#undef FSTAT
}

auto Sync(std::FILE* _fp) -> bool { return ::fsync(::fileno(_fp)) == 0; }

}  // namespace io_inner
}  // namespace hare

#endif