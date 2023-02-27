#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/base/detail/non_copyable.h>
#include <hare/base/util.h>
#include <hare/base/timestamp.h>
#include <hare/net/buffer.h>

#include <memory>

namespace hare {
namespace net {

    class TcpServe;
    class TcpClient;
    class TcpSessionPrivate;
    class HARE_API TcpSession : public NonCopyable
                              , public std::enable_shared_from_this<TcpSession> {
        friend class TcpServe;
        friend class TcpClient;

        TcpSessionPrivate* p_ { nullptr };

    public:
        virtual ~TcpSession();

        void setHighWaterMark(std::size_t high_water = 64 * 1024 * 1024);

    protected:
        virtual void connection(int32_t flag) {}
        virtual void writeComplete() {}
        virtual void highWaterMark() {}
        virtual void read(Buffer& b, Timestamp& ts) {}

    private:
        TcpSession(TcpSessionPrivate* p);

    };

    using STcpSession = std::shared_ptr<TcpSession>;

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SESSION_H_