#include <hare/base/util/any.h>

#include "base/fwd-inl.h"

namespace hare {
namespace util_inner {
void ThrowBadAnyCast() { HARE_INTERNAL_FATAL("bad any cast"); }
}  // namespace util_inner

}  // namespace hare