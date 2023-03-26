#ifndef _HARE_BASE_NON_COPYABLE_H_
#define _HARE_BASE_NON_COPYABLE_H_

#include <hare/base/util/util.h>

namespace hare {

class HARE_API NonCopyable {
public:
    NonCopyable(const NonCopyable&) = delete;
    auto operator=(const NonCopyable&) -> NonCopyable& = delete;

protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};

} // namespace hare

#endif // !_HARE_BASE_NON_COPYABLE_H_