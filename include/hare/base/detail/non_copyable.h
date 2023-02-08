#ifndef _HARE_BASE_NON_COPYABLE_H_
#define _HARE_BASE_NON_COPYABLE_H_

namespace hare {

class NonCopyable {
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable operator=(const NonCopyable&) = delete;

protected:
    NonCopyable() = default;
    ~NonCopyable() = default;
};

}

#endif // !_HARE_BASE_NON_COPYABLE_H_