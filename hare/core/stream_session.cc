#include <hare/core/stream_session.h>

namespace hare {
namespace core {

    StreamSession::~StreamSession()
    {
        
    }

    StreamSession::StreamSession(net::TcpSessionPrivate* tsp)
        : TcpSession(tsp)
    {

    }
    
} // namespace core
} // namespace hare 