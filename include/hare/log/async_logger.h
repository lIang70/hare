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

#ifndef _HARE_LOG_ASYNC_LOGGER_H_
#define _HARE_LOG_ASYNC_LOGGER_H_

#include <hare/base/util/queue.h>
#include <hare/base/util/thread_pool.h>
#include <hare/log/details/async_msg.h>
#include <hare/log/logging.h>

#include <utility>

namespace hare {
namespace log {

    HARE_CLASS_API
    class HARE_API AsyncLogger : public Logger {
        util::ThreadPool<details::AsyncMsg> thread_pool_;
        Policy msg_policy_ { util::Policy::BLOCK_RETRY };

    public:
        template <typename Iter>
        HARE_INLINE
        AsyncLogger(std::string _unique_name, const Iter& begin, const Iter& end,
            std::size_t _max_msg, std::size_t _thr_n)
            : Logger(std::move(_unique_name), begin, end)
            , thread_pool_(_max_msg, _thr_n, std::bind(&AsyncLogger::HandleMsg, this, std::placeholders::_1))
        {
            thread_pool_.Start([] {}, [] {});
        }

        HARE_INLINE
        AsyncLogger(std::string _unique_name, BackendList _backends, std::size_t _max_msg, std::size_t _thr_n)
            : AsyncLogger(std::move(_unique_name), _backends.begin(), _backends.end(), 
            _max_msg, _thr_n)
        {
        }

        HARE_INLINE
        AsyncLogger(std::string _unique_name, Ptr<Backend> _backend, std::size_t _max_msg, std::size_t _thr_n)
            : AsyncLogger(std::move(_unique_name), BackendList { std::move(_backend) }, 
            _max_msg, _thr_n)
        {
        }

        ~AsyncLogger() override;

        HARE_INLINE void set_policy(Policy _policy) { msg_policy_ = _policy; }

        void Flush() override;

    private:
        void SinkIt(details::Msg& _msg) override;

        auto HandleMsg(details::AsyncMsg& _msg) -> bool;
    };

} // namespace log
} // namespace hare

#endif // _HARE_LOG_ASYNC_LOGGER_H_