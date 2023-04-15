#include <hare/base/log/async.h>

#include "hare/base/log/file.h"
#include <hare/base/exception.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/system_info.h>

#include <cassert>
#include <functional>

namespace hare {
namespace log {

    async::async(int64_t _roll_size, std::string _name, int32_t _flush_interval)
        : name_(std::move(_name))
        , roll_size_(_roll_size)
        , flush_interval_(_flush_interval)
    {
        current_block_->bzero();
        next_block_->bzero();
        blocks_.reserve(BLOCK_NUMBER);
    }

    async::~async()
    {
        if (running_) {
            stop();
        }
    }

    void async::append(const char* _log_line, int32_t _size)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (current_block_->avail() > _size) {
            current_block_->append(_log_line, _size);
        } else {
            blocks_.push_back(std::move(current_block_));

            if (next_block_) {
                current_block_ = std::move(next_block_);
            } else {
                current_block_.reset(new fixed_block); // Rarely happens
            }
            current_block_->append(_log_line, _size);
            cv_.notify_all();
        }
    }

    void async::start()
    {
        running_ = true;
        thread_ = std::make_shared<std::thread>(std::bind(&async::run, this));
        latch_.await();
    }

    void async::stop()
    {
        running_ = false;
        cv_.notify_all();
        if (thread_ && thread_->joinable()) {
            thread_->join();
        }
    }

    void async::run()
    {
        if (!running_) {
            throw exception("LOG_ASYNC is not running.");
        }

        util::set_tname("LOG_ASYNC");

        latch_.count_down();

        log::file output(name_, roll_size_, false);
        fixed_block::ptr new_block_1(new fixed_block);
        fixed_block::ptr new_block_2(new fixed_block);
        new_block_1->bzero();
        new_block_2->bzero();

        blocks block_2_write;
        block_2_write.reserve(BLOCK_NUMBER);
        while (running_) {
            assert(new_block_1 && new_block_1->length() == 0);
            assert(new_block_2 && new_block_2->length() == 0);
            assert(block_2_write.empty());

            {
                std::unique_lock<std::mutex> lock(mutex_);
                // unusual usage!
                if (blocks_.empty()) {
                    cv_.wait_for(lock, std::chrono::seconds(flush_interval_));
                }
                blocks_.push_back(std::move(current_block_));
                current_block_ = std::move(new_block_1);
                block_2_write.swap(blocks_);
                if (!next_block_) {
                    next_block_ = std::move(new_block_2);
                }
            }

            assert(!block_2_write.empty());

            if (block_2_write.size() > BLOCK_NUMBER * 2) {
                std::array<char, static_cast<int64_t>(HARE_SMALL_FIXED_SIZE) * 4> buf;
                ::snprintf(buf.data(), buf.size(), "[WARN ] dropped log messages at [%s], %zd larger buffers\n",
                    timestamp::now().to_fmt().c_str(),
                    block_2_write.size() - 2);
                fputs(buf.data(), stderr);
                output.append(buf.data(), buf.size());
                block_2_write.erase(block_2_write.begin() + 2, block_2_write.end());
            }

            for (const auto& buffer : block_2_write) {
                // FIXME: use unbuffered stdio FILE ? or use ::writev ?
                output.append(buffer->begin(), buffer->length());
            }

            if (block_2_write.size() > 2) {
                // drop non-bzero-ed buffers, avoid trashing
                block_2_write.resize(2);
            }

            if (!new_block_1) {
                assert(!block_2_write.empty());
                new_block_1 = std::move(block_2_write.back());
                block_2_write.pop_back();
                new_block_1->reset();
            }

            if (!new_block_2) {
                assert(!block_2_write.empty());
                new_block_2 = std::move(block_2_write.back());
                block_2_write.pop_back();
                new_block_2->reset();
            }

            block_2_write.clear();
            output.flush();
        }
        output.flush();
    }

} // namespace log
} // namespace hare