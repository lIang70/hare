#ifndef _HARE_BASE_NON_COPYABLE_H_
#define _HARE_BASE_NON_COPYABLE_H_

#include <hare/base/util.h>

namespace hare {

class HARE_API NonCopyable {
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable operator=(const NonCopyable&) = delete;

protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};

}

#endif // !_HARE_BASE_NON_COPYABLE_H_