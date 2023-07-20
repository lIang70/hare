/**
 * @file hare/log/details/msg.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with msg.h
 * @version 0.1-beta
 * @date 2023-07-12
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_LOG_DETAILS_MSG_H_
#define _HARE_LOG_DETAILS_MSG_H_

#include <hare/base/time/timestamp.h>
#include <hare/base/time/timezone.h>
#include <hare/base/util/non_copyable.h>
#include <hare/log/details/dummy.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

#include <unordered_map>

#if !defined(HARE_EOL)
#ifdef H_OS_WIN32
#define HARE_EOL "\r\n"
#else
#define HARE_EOL "\n"
#endif
#endif

#if !defined(HARE_SLASH)
#ifdef H_OS_WIN32
#define HARE_SLASH '\\'
#else
#define HARE_SLASH '/'
#endif
#endif

namespace hare {
namespace log {

    /**
     * @brief The enumeration of log level.
     *
     **/
    using LEVEL = enum : std::int8_t {
        LEVEL_TRACE,
        LEVEL_DEBUG,
        LEVEL_INFO,
        LEVEL_WARNING,
        LEVEL_ERROR,
        LEVEL_FATAL,
        LEVEL_NBRS
    };

#ifndef HARELOG_USING_ATOMIC_LEVELS
    using level_t = details::dummy_atomic_t<std::int8_t>;
#else
    using level_t = std::atomic<std::int8_t>;
#endif

#if !defined(HARELOG_LEVEL_NAMES)
#define HARELOG_LEVEL_NAMES                                           \
    {                                                                 \
        "trace", "debug", "info", "warning", "error", "fatal", "nbrs" \
    }
#endif

    static constexpr const char* level_name[] HARELOG_LEVEL_NAMES;

    HARE_INLINE
    constexpr const char* to_str(LEVEL _level)
    {
        return level_name[_level];
    }

    HARE_INLINE
    LEVEL from_str(const std::string& name)
    {
        static std::unordered_map<std::string, LEVEL> name_to_level = {
            { level_name[0], LEVEL_TRACE }, // trace
            { level_name[1], LEVEL_DEBUG }, // debug
            { level_name[2], LEVEL_INFO }, // info
            { level_name[3], LEVEL_WARNING }, // warning
            { level_name[4], LEVEL_ERROR }, // error
            { level_name[5], LEVEL_FATAL }, // critical
            { level_name[6], LEVEL_NBRS } // nbrs
        };

        auto iter = name_to_level.find(name);
        return iter != name_to_level.end() ? iter->second : LEVEL_NBRS;
    }

    HARE_CLASS_API
    struct HARE_API source_loc {
        const char* filename_ { nullptr };
        const char* funcname_ { nullptr };
        std::int32_t line_ { 0 };

        constexpr source_loc() = default;
        HARE_INLINE source_loc(const char* _filename, std::int32_t _line, const char* _funcname)
            : funcname_ { _funcname }
            , line_ { _line }
        {
            const auto* slash = std::strrchr(_filename, HARE_SLASH);
            filename_ = slash != nullptr ? slash + 1 : _filename;
        }

        HARE_INLINE
        auto empty() const noexcept -> bool
        {
            return line_ == 0;
        }
    };

    namespace details {

        using msg_buffer_t = fmt::basic_memory_buffer<char, 256>;

        HARE_CLASS_API
        struct HARE_API msg : public util::non_copyable {
            const std::string* name_ {};
            const timezone* timezone_ {};
            std::int8_t level_ { LEVEL_NBRS };
            std::uint64_t tid_ { 0 };
            std::uint64_t id_ { 0 };
            timestamp stamp_ { timestamp::now() };
            msg_buffer_t raw_ {};
            source_loc loc_ {};

            msg() = default;
            virtual ~msg() = default;

            msg(const std::string* _name, const timezone* _timezone, LEVEL _level, source_loc _loc);

            HARE_INLINE
            msg(msg&& _other) noexcept
            { move(_other); }

            HARE_INLINE
            auto operator=(msg&& _other) noexcept -> msg&
            { move(_other); return (*this); }

            HARE_INLINE
            void move(msg& _other) noexcept
            {
                name_ = _other.name_;
                timezone_ = _other.timezone_;
                level_ = _other.level_;
                tid_ = _other.tid_;
                id_ = _other.id_;
                stamp_ = _other.stamp_;
                raw_ = std::move(_other.raw_);
                loc_ = _other.loc_;
            }
        };

        HARE_API void format_msg(msg& _msg, msg_buffer_t& _fotmatted);

    } // namespace details
} // namespace log
} // namespace hare

#endif // _HARE_LOG_DETAILS_MSG_H_
