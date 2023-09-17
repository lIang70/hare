#include "store.h"

namespace hare {

auto ThreadStoreData() -> thread_inner::Store::Data& {
  static thread_local thread_inner::Store thread_store{};
  return thread_store();
}

}  // namespace hare