#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/base/util.h>

namespace hare {
namespace net {

    class HARE_API TcpSession {
        class Data;
        Data* d_ { nullptr };

    public:
        TcpSession();
        virtual ~TcpSession();

        
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SESSION_H_