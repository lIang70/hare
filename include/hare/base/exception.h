#ifndef _HARE_BASE_EXCEPTION_H_
#define _HARE_BASE_EXCEPTION_H_

#include <hare/base/util/util.h>

#include <exception>
#include <string>

namespace hare {

class HARE_API Exception : std::exception {
    std::string what_ {};
    std::string stack_ {};

public:
    explicit Exception(std::string what);
    ~Exception() override;

    const char* what() const noexcept override;

    const char* stackTrace() const noexcept;
};

}

#endif // !_HARE_BASE_EXCEPTION_H_