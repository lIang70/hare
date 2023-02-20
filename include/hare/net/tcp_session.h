#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/base/detail/non_copyable.h>
#include <hare/base/util.h>
#include <hare/net/buffer.h>

namespace hare {
namespace net {

    class TcpSessionPrivate;
    class HARE_API TcpSession : public NonCopyable {
        TcpSessionPrivate* p_ { nullptr };

    public:
        virtual ~TcpSession();

    protected:
        virtual void connection(int32_t flag) {}
        virtual void writeComplete() {}
        virtual void highWaterMark() {}
        virtual void read(Buffer& b, Timestamp& ts) {}

    private:
        TcpSession(TcpSessionPrivate* p);

    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SESSION_H_