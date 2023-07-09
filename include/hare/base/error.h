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

using HARE_ERROR = enum : int {
    HARE_ERROR_ILLEGAL,
    HARE_ERROR_SUCCESS,

    // Base
    HARE_ERROR_SET_THREAD_NAME,

    HARE_ERRORS_NBR
};

class HARE_API error {
    int error_code_ { HARE_ERROR_SUCCESS };
    int system_code_ { 0 };

public:
    explicit error(int error_code);
    virtual ~error() = default;

    /**
     * @brief Get the error code of hare.
     *
     **/
    inline auto code() const -> int { return error_code_; }

    /**
     * @brief Get the system code 'errno'.
     *
     **/
    inline auto system_code() const -> int { return system_code_; }

    inline explicit operator bool() const { return error_code_ == HARE_ERROR_SUCCESS; }
    inline auto operator==(HARE_ERROR error) const -> bool { return error_code_ == error; }

    /**
     * @brief Get the description of the error code.
     *
     **/
    virtual auto description() const -> const char*;
};

} // namespace hare

#endif // !_HARE_BASE_ERROR_H_