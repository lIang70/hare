/**
 * @file hare/base/util/non_copyable.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with non_copyable.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_UTIL_NON_COPYABLE_H_
#define _HARE_BASE_UTIL_NON_COPYABLE_H_

#include <hare/base/fwd.h>

namespace hare {
namespace util {

HARE_CLASS_API
class HARE_API NonCopyable {
 public:
  NonCopyable(const NonCopyable&) = delete;
  auto operator=(const NonCopyable&) -> NonCopyable& = delete;

 protected:
  NonCopyable() = default;
  ~NonCopyable() = default;
};

HARE_CLASS_API
class HARE_API NonCopyableNorMovable {
 public:
  NonCopyableNorMovable(const NonCopyable&) = delete;
  NonCopyableNorMovable(NonCopyable&&) noexcept = delete;
  auto operator=(const NonCopyableNorMovable&)
      -> NonCopyableNorMovable& = delete;
  auto operator=(NonCopyableNorMovable&&) noexcept
      -> NonCopyableNorMovable& = delete;

 protected:
  NonCopyableNorMovable() = default;
  ~NonCopyableNorMovable() = default;
};

}  // namespace util
}  // namespace hare

#endif  // _HARE_BASE_UTIL_NON_COPYABLE_H_