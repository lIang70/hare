/**
 * @file hare/base/error.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with error.h
 * @version 0.1-beta
 * @date 2023-04-05
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_ERROR_H_
#define _HARE_BASE_ERROR_H_

#include <hare/base/fwd.h>

namespace hare {

using HARE_ERROR = enum : int32_t {
    HARE_ERROR_ILLEGAL,
    HARE_ERROR_SUCCESS,

    // Base
    HARE_ERROR_SET_THREAD_NAME,

    HARE_ERRORS_NBR
};

HARE_CLASS_API
class HARE_API error {
    std::int32_t error_code_ { HARE_ERROR_SUCCESS };
    std::int32_t system_code_ { 0 };

public:
    explicit error(std::int32_t error_code);
    virtual ~error() = default;

    /**
     * @brief Get the error code of hare.
     *
     **/
    HARE_INLINE
    auto code() const -> std::int32_t { return error_code_; }

    /**
     * @brief Get the system code 'errno'.
     *
     **/
    HARE_INLINE
    auto system_code() const -> std::int32_t { return system_code_; }

    HARE_INLINE
    explicit operator bool() const { return error_code_ == HARE_ERROR_SUCCESS; }
    HARE_INLINE
    auto operator==(HARE_ERROR error) const -> bool { return error_code_ == error; }

    /**
     * @brief Get the description of the error code.
     *
     **/
    virtual auto description() const -> const char*;
};

} // namespace hare

#endif // !_HARE_BASE_ERROR_H_