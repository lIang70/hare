#include "hare/base/log/file.h"
#include "hare/base/log/util.h"
#include <hare/base/log/async.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/exception.h>

#include <cassert>
#include <utility>

namespace hare {
namespace log {

    Async::Async(const std::string& name, int64_t roll_size, int32_t flush_interval)
        : name_(std::move(name))
        , roll_size_(roll_size)
        , flush_interval_(flush_interval)
        , thread_(std::bind(&Async::run, this), "LOG_ASYNC")
    {
        current_block_->bzero();
        next_block_->bzero();
        blocks_.reserve(16);
    }

    Async::~Async()
    {
        if (running_) {
            stop();
        }
    }

    void Async::append(const char* log_line, int32_t size)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (current_block_->avail() > size) {
            current_block_->append(log_line, size);
        } else {
            blocks_.push_back(std::move(current_block_));

            if (next_block_) {
                current_block_ = std::move(next_block_);
            } else {
                current_block_.reset(new Block); // Rarely happens
            }
            current_block_->append(log_line, size);
            cv_.notify_all();
        }
    }

    void Async::start()
    {
        running_.exchange(true);
        thread_.start();
        latch_.await();
    }

    void Async::stop()
    {
        running_.exchange(false);
        cv_.notify_all();
        thread_.join();
    }

    void Async::run()
    {
        if (!running_)
            throw Exception("LOG_ASYNC is not running.");
        latch_.countDown();

        log::File output(name_, roll_size_, false);
        Block::Ptr new_block_1(new Block);
        Block::Ptr new_block_2(new Block);
        new_block_1->bzero();
        new_block_2->bzero();

        Blocks block_2_write;
        block_2_write.reserve(16);
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

            if (block_2_write.size() > 32) {
                char buf[256];
                snprintf(buf, sizeof(buf), "[Warning ] Dropped log messages at [%s], %zd larger buffers\n",
                    Timestamp::now().toFormattedString().c_str(),
                    block_2_write.size() - 2);
                fputs(buf, stderr);
                output.append(buf, static_cast<int>(strlen(buf)));
                block_2_write.erase(block_2_write.begin() + 2, block_2_write.end());
            }

            for (const auto& buffer : block_2_write) {
                // FIXME: use unbuffered stdio FILE ? or use ::writev ?
                output.append(buffer->data(), buffer->length());
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