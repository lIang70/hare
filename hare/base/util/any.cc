#include <hare/base/util/any.h>
#include <hare/base/exception.h>

namespace hare {
namespace util {
    namespace detail {
        void ThrowBadAnyCast()
        {
            throw Exception("bad any cast");
        }
    } // namespace detail

} // namespace util
} // namespace hare