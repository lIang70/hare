#include <hare/log/async_logger.h>

#include "base/fwd-inl.h"

namespace hare {
namespace log {

AsyncLogger::~AsyncLogger() {
  auto thr_cnt = thread_pool_.ThreadSize();
  for (decltype(thr_cnt) i = 0; i < thr_cnt; ++i) {
    thread_pool_.Post(detail::AsyncMsg(detail::AsyncMsg::TERMINATE),
                      Policy::BLOCK_RETRY);
  }
  thread_pool_.Join();
}

void AsyncLogger::Flush() {
  thread_pool_.Post(detail::AsyncMsg(detail::AsyncMsg::FLUSH),
                    Policy::BLOCK_RETRY);
}

void AsyncLogger::SinkIt(detail::Msg& _msg) {
  thread_pool_.Post({_msg, detail::AsyncMsg::LOG}, msg_policy_);
}

auto AsyncLogger::HandleMsg(detail::AsyncMsg& _msg) -> bool {
  switch (_msg.type_) {
    case detail::AsyncMsg::LOG: {
      IncreaseMsgId(_msg);

      detail::msg_buffer_t formatted{};
      detail::FormatMsg(_msg, formatted);

      for (auto& backend : backends_) {
        if (backend->Check(_msg.level_)) {
          backend->Log(formatted, static_cast<Level>(_msg.level_));
        }
      }

      if (ShouldFlushOn(_msg)) {
        Flush();
      }
      return true;
    }
    case detail::AsyncMsg::FLUSH:
      try {
        for (auto& backend : backends_) {
          backend->Flush();
        }
      } catch (const hare::Exception& e) {
        error_handle_(ERROR_MSG, e.what());
      } catch (const std::exception& e) {
        error_handle_(ERROR_MSG, e.what());
      } catch (...) {
        error_handle_(ERROR_MSG, "Unknown exeption in logger");
      }
      return true;
      break;
    case detail::AsyncMsg::TERMINATE:
      break;
    default:
      HARE_ASSERT(false);
      break;
  }
  return false;
}

}  // namespace log
}  // namespace hare