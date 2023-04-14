/**
 * @file hare/base/logging.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the macro, class and functions
 *   associated with logging.h
 * @version 0.1-beta
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_BASE_LOGGING_H_
#define _HARE_BASE_LOGGING_H_

#include <cstddef>
#include <hare/base/log/stream.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/time/timezone.h>

#include <functional>

namespace hare {
namespace log {

    /**
     * @brief The enumeration of log level.
     *
     **/
    using LEVEL = enum : int8_t {
        LEVEL_TRACE,
        LEVEL_DEBUG,
        LEVEL_INFO,
        LEVEL_WARNING,
        LEVEL_ERROR,
        LEVEL_FATAL,
        LEVEL_NBRS
    };

    /**
     * @brief Get the infomation of error number.
     *
     * @param errorno The error number.
     * @return const char* The infomation of error.
     **/
    HARE_API auto errnostr(int errorno) -> const char*;

} // namespace log

/**
 * @brief The interface class of log.
 **/
class HARE_API logger {
public:
    using output = std::function<void(const char*, size_t)>;
    using flush = std::function<void()>;

    class HARE_API file_path {
        const char* data_ { nullptr };
        size_t size_ { 0 };

    public:
        template <size_t Length>
        file_path(const char (&_file_name)[Length])
            : data_(_file_name)
            , size_(Length - 1)
        {
            const char* slash = ::strrchr(data_, '/'); // builtin function
            if (slash) {
                data_ = slash + 1;
                size_ -= static_cast<int32_t>(data_ - _file_name);
            }
        }

        explicit file_path(const char* _file_name)
            : data_(_file_name)
        {
            const char* slash = ::strrchr(_file_name, '/');
            if (slash != nullptr) {
                data_ = slash + 1;
            }
            size_ = static_cast<int32_t>(strlen(data_));
        }

        inline auto data() const -> const char* { return data_; }
        inline auto size() const -> size_t { return size_; }
    };

private:
    class HARE_API data {
        timestamp time_ {};
        log::stream stream_ {};
        log::LEVEL level_ {};
        int line_ {};
        file_path base_name_;

    public:
        data(log::LEVEL _level, int _old_errno, const file_path& _file, int _line);

        void format_time();
        void finish();

        friend class logger;
    };
    struct data data_;

public:
    logger(file_path _file, int32_t _line);
    logger(file_path _file, int32_t _line, log::LEVEL _level);
    logger(file_path _file, int32_t _line, log::LEVEL _level, const char* _func);
    logger(file_path _file, int32_t _line, bool _to_abort);
    ~logger();

    auto stream() -> log::stream& { return data_.stream_; }

    /**
     * @brief Set/Get the global level of log.
     *
     **/
    static void set_level(log::LEVEL _level);
    static auto level() -> log::LEVEL;

    /**
     * @brief Set the global output function of log.
     *
     **/
    static void set_output(output _output);

    /**
     * @brief Set the global flush function of log.
     *
     **/
    static void set_flush(flush _flush);

    /**
     * @brief Set the global time zone of log.
     *
     **/
    static void set_timezone(const timezone& _time_zone);
};

template <typename Ty>
auto check_not_null(logger::file_path _file, int _line, const char* _names, Ty* _ptr) -> Ty*
{
    if (_ptr == nullptr) {
        logger(_file, _line, hare::log::LEVEL_FATAL).stream() << _names;
    }
    return _ptr;
}

} // namespace hare

#define LOG_TRACE() \
    hare::logger(__FILE__, __LINE__, hare::log::LEVEL_TRACE, __func__).stream()
#define LOG_DEBUG() \
    hare::logger(__FILE__, __LINE__, hare::log::LEVEL_DEBUG, __func__).stream()
#define LOG_INFO() \
    hare::logger(__FILE__, __LINE__, hare::log::LEVEL_INFO, __func__).stream()
#define LOG_WARNING() \
    hare::logger(__FILE__, __LINE__, hare::log::LEVEL_WARNING, __func__).stream()
#define LOG_ERROR() \
    hare::logger(__FILE__, __LINE__, hare::log::LEVEL_ERROR, __func__).stream()
#define LOG_FATAL() \
    hare::logger(__FILE__, __LINE__, hare::log::LEVEL_FATAL, __func__).stream()
#define SYS_ERROR() \
    hare::logger(__FILE__, __LINE__, false).stream()
#define SYS_FATAL() \
    hare::logger(__FILE__, __LINE__, true).stream()

#ifdef HARE_DEBUG
#define HARE_ASSERT(val, what)                                     \
    do {                                                           \
        if (!(val)) {                                              \
            hare::logger(__FILE__, __LINE__, true).stream()        \
                << "Condition[" << #val << "] failed. " << (what); \
        }                                                          \
    } while (0)
#else
#define HARE_ASSERT(val, what) H_UNUSED(val), H_UNUSED(what)
#endif

/**
 * @brief Check that the input is non NULL.
 *   This very useful in constructor initializer lists.
 **/
#define HARE_CHECK_NULL(val) \
    ::hare::check_not_null(__FILE__, __LINE__, "\'" #val "\' must be non NULL", (val))

#endif // !_HARE_BASE_LOGGING_H_