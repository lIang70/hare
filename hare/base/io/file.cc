#include <hare/base/io/file.h>

#include "base/fwd-inl.h"

#ifndef H_OS_WIN
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <Windows.h>
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#define fileno _fileno
#endif

namespace hare {
namespace io {

namespace file_inner {

auto Open(std::FILE** _fp, const filename_t& _filename, const filename_t& _mode)
    -> bool {
#if defined(H_OS_WIN32) && HARE_WCHAR_FILENAME
  *_fp = ::_wfsopen(FilenameToStr(_filename).c_str(),
                    FilenameToStr(_mode).c_str(), _SH_DENYWR);
#elif defined(H_OS_WIN32)
  *_fp = ::_fsopen(FilenameToStr(_filename).c_str(),
                   FilenameToStr(_mode).c_str(), _SH_DENYWR);
#else  // unix
  *_fp =
      ::fopen(FilenameToStr(_filename).c_str(), FilenameToStr(_mode).c_str());
#endif
  return *_fp != nullptr;
}

auto Exists(const filename_t& _filepath) -> bool {
#if defined(H_OS_LINUX)  // common linux/unix all have the stat system call
  struct stat buffer {};
  return (::stat(FilenameToStr(_filepath).c_str(), &buffer) == 0);
#elif defined(H_OS_WIN32)
  WIN32_FIND_DATA wfd{};
  auto hFind = ::FindFirstFile(FilenameToStr(_filepath).data(), &wfd);
  auto ret = hFind != INVALID_HANDLE_VALUE;
  ::CloseHandle(hFind);
  return ret;
#endif
}

auto Remove(const filename_t& _filepath) -> bool {
  return std::remove(FilenameToStr(_filepath).c_str()) == 0;
}

auto Size(std::FILE* _fp) -> std::size_t {
  if (_fp == nullptr) {
    return 0;
  }
  auto fd = ::fileno(_fp);
#ifndef H_OS_WIN
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
#else
  auto size = ::_filelengthi64(fd);
  return size < 0 ? 0 : static_cast<std::size_t>(size);
#endif
}

auto Sync(std::FILE* _fp) -> bool {
#if defined(H_OS_LINUX)
  return ::fsync(::fileno(_fp)) == 0;
#elif defined(H_OS_WIN32)

  return false;
#endif
}

}  // namespace file_inner

}  // namespace io
}  // namespace hare