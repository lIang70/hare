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

#include <hare/base/util.h>

#include <exception>
#include <string>

namespace hare {

class HARE_API exception : public std::exception {
    std::string what_ {};
    std::string stack_ {};

public:
    explicit exception(std::string what) noexcept;
    ~exception() noexcept override = default;
    exception(const exception&) = default;
    exception& operator=(const exception&) = default;
    exception(exception&&) = default;
    exception& operator=(exception&&) = default;

    inline auto what() const noexcept -> const char* override { return what_.c_str(); }

    inline auto stack_trace() const noexcept -> const char* { return stack_.c_str(); }
};

} // namespace hare

#endif // !_HARE_BASE_EXCEPTION_H_