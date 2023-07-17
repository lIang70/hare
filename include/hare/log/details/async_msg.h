/**
 * @file hare/log/async_logger.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with async_logger.h
 * @version 0.1-beta
 * @date 2023-07-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_ASYNC_MSG_H_
#define _HARE_LOG_ASYNC_MSG_H_

#include <hare/log/details/msg.h>

namespace hare {
namespace log {
    namespace details {

        HARE_CLASS_API
        struct HARE_API async_msg : public msg {
            enum type { LOG, FLUSH, TERMINATE };
            type type_ { LOG };

            async_msg() = default;

            explicit async_msg(type _type)
                : type_(_type)
            {
            }

            async_msg(msg& _msg, type _type)
                : msg(std::move(_msg))
                , type_(_type)
            {
            }

            HARE_INLINE
            async_msg(async_msg&& _other) noexcept
            { move(_other); }

            HARE_INLINE
            auto operator=(async_msg&& _other) noexcept -> async_msg&
            { move(_other); return (*this); }

            void move(async_msg& other) noexcept;
        };

    } // namespace details
} // namespace log
} // namespace hare

#endif // _HARE_LOG_ASYNC_MSG_H_