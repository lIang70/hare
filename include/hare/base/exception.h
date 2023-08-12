/**
 * @file hare/base/exception.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with exception.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_EXCEPTION_H_
#define _HARE_BASE_EXCEPTION_H_

#include <hare/base/fwd.h>

#include <exception>

namespace hare {

HARE_CLASS_API
class HARE_API Exception : public std::exception {
    hare::detail::Impl *impl_ {};

public:
    explicit Exception(std::string what) noexcept;
    ~Exception() noexcept override;
    Exception(const Exception&) = default;
    Exception& operator=(const Exception&) = default;
    Exception(Exception&&) = default;
    Exception& operator=(Exception&&) = default;

    auto what() const noexcept -> const char* override;
    auto StackTrace() const noexcept -> const char*;
};

} // namespace hare

#endif // _HARE_BASE_EXCEPTION_H_
