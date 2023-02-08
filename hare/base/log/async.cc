#include "hare/base/log/file.h"
#include "hare/base/util/count_down_latch.h"
#include <hare/base/detail/log_async.h>
#include <hare/base/exception.h>
#include <hare/base/thread.h>
#include <hare/base/timestamp.h>

#include <assert.h>

namespace hare {
namespace log {

    class Async::Data {
    public:
        Async* async_ { nullptr };
        Thread thread_;
        util::CountDownLatch latch_ { 1 };
        std::mutex mutex_ {};
        std::condition_variable cv_ {};
        BPtr current_block_ { new Block };
        BPtr next_block_ { new Block };
        BlockV blocks_ {};

        Data(Async* async)
            : async_(async)
            , thread_(std::bind(&Async::Data::run, this), "log::Async")
        {
            if (!async_)
                throw Exception("log::Async is NULL.");
            current_block_->bzero();
            next_block_->bzero();
            blocks_.reserve(16);
        }

        void run()
        {
            if (!async_->running_)
                throw Exception("log::Async is not running.");
            latch_.countDown();

            log::File output(async_->name_, async_->roll_size_, false);
            BPtr new_block_1(new Block);
            BPtr new_block_2(new Block);
            new_block_1->bzero();
            new_block_2->bzero();

            BlockV block_2_write;
            block_2_write.reserve(16);
            while (async_->running_) {
                assert(new_block_1 && new_block_1->length() == 0);
                assert(new_block_2 && new_block_2->length() == 0);
                assert(block_2_write.empty());

                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    // unusual usage!
                    if (blocks_.empty()) {
                        cv_.wait_for(lock, std::chrono::seconds(async_->flush_interval_));
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
    };

    Async::Async(const std::string& name, int64_t roll_size, int32_t flush_interval)
        : name_(name)
        , roll_size_(roll_size)
        , flush_interval_(flush_interval)
        , data_(new Data(this))
    {
        if (!data_)
            throw Exception("Cannot create a data for log::Async");
    }

    void Async::append(const char* log_line, int32_t size)
    {
        std::unique_lock<std::mutex> lock(data_->mutex_);
        if (data_->current_block_->avail() > size) {
            data_->current_block_->append(log_line, size);
        } else {
            data_->blocks_.push_back(std::move(data_->current_block_));

            if (data_->next_block_) {
                data_->current_block_ = std::move(data_->next_block_);
            } else {
                data_->current_block_.reset(new Block); // Rarely happens
            }
            data_->current_block_->append(log_line, size);
            data_->cv_.notify_all();
        }
    }

    void Async::start()
    {
        running_.exchange(true);
        data_->thread_.start();
        data_->latch_.await();
    }

    void Async::stop()
    {
        running_.exchange(false);
        data_->cv_.notify_all();
        data_->thread_.join();
    }

} // namespace log
} // namespace hare