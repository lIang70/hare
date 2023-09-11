#include <hare/base/exception.h>
#include <hare/base/util/any.h>

#include "base/fwd-inl.h"

namespace hare {
namespace util {
namespace detail {
void ThrowBadAnyCast() { HARE_INTERNAL_FATAL("bad any cast"); }
}  // namespace detail

}  // namespace util
}  // namespace hare