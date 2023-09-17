#ifndef _HARE_BASE_THREAD_STORE_H_
#define _HARE_BASE_THREAD_STORE_H_

#include <hare/base/fwd.h>

#include <sstream>
#include <thread>

namespace hare {

class Cycle;
class Event;

namespace thread_inner {

class Store {
 public:
  struct Data {
    std::uint64_t tid{0};
    Cycle* cycle{nullptr};
  };

  Store() : data_(new Data) {
    if (HARE_PREDICT_FALSE(data_->tid == 0)) {
      std::ostringstream oss{};
      oss << std::this_thread::get_id();
      data_->tid = std::stoull(oss.str());
    }
  }

  ~Store() { delete data_; }

  auto operator()() -> Data& { return *data_; }

 private:
  Data* data_{nullptr};
};

}  // namespace thread_inner

HARE_API auto ThreadStoreData() -> thread_inner::Store::Data&;

}  // namespace hare

#endif  // _HARE_BASE_THREAD_STORE_H_