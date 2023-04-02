//! 
//! @file hare/base/exception.h
//! @author l1ang70 (gog_017@outlook.com)
//! @brief Describe the class associated with
//!   exception.
//! @version 0.1-beta
//! @date 2023-02-09
//! 
//! @copyright Copyright (c) 2023
//! 

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

    auto what() const noexcept -> const char* override;

    auto stackTrace() const noexcept -> const char*;
};

} // namespace hare

#endif // !_HARE_BASE_EXCEPTION_H_