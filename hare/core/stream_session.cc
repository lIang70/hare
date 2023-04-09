#include <hare/base/logging.h>
#include <hare/core/stream_client.h>
#include <hare/core/stream_session.h>
#include <hare/net/util.h>

namespace hare {
namespace core {

    StreamSession::~StreamSession()
    {
        LOG_DEBUG() << "Session[" << name() << "] was destroyed...";
    }

    StreamSession::StreamSession(net::TcpSessionPrivate* tsp)
        : TcpSession(tsp)
    {
    }

    void StreamSession::connection(int32_t flag)
    {
        switch (flag) {
        case net::EVENT_CONNECTED:
            LOG_DEBUG() << "Session[" << name() << "] is connected.";
            break;
        case net::EVENT_CLOSED:
            LOG_INFO() << "Session[" << name() << "] is closed.";
            if (close_session_) {
                /// FIXME queueInCycle
                close_session_(socket());
            }
            break;
        case net::EVENT_ERROR:
            forceClose();
            break;
        default:
            LOG_INFO() << "Session[" << name() << "] unknown op.";
            break;
        }
    }

    void StreamSession::writeComplete()
    {
    }

    void StreamSession::highWaterMark()
    {
    }

    void StreamSession::read(net::Buffer& buffer, const Timestamp& time)
    {
        LOG_DEBUG() << "Recv data[" << buffer.length() << " B] from session[" << name() << "] in " << time.toFormattedString(true);
        if (!client_.expired()) {
            if (auto client = client_.lock()) {
                client->process(buffer, time);
            }
        }
    }

} // namespace core
} // namespace hare