#include "base/fwd-inl.h"
#include <hare/base/util/any.h>
#include <hare/base/exception.h>

namespace hare {
namespace util {
    namespace detail {
        void ThrowBadAnyCast()
        {
            HARE_INTERNAL_FATAL("bad any cast");
        }
    } // namespace detail

} // namespace util
} // namespace hare