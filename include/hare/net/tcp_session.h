#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/base/detail/non_copyable.h>
#include <hare/base/util.h>
#include <hare/base/timestamp.h>
#include <hare/net/buffer.h>

#include <memory>

namespace hare {
namespace net {

    class TcpSessionPrivate;
    class HARE_API TcpSession : public NonCopyable
                              , public std::enable_shared_from_this<TcpSession> {
        friend class TcpServe;
        friend class TcpServePrivate;

        TcpSessionPrivate* p_ { nullptr };

    public:
        virtual ~TcpSession();

        const std::string& name() const;
        const HostAddress& local_address() const;
        const HostAddress& peer_address() const;

        void setHighWaterMark(std::size_t high_water = 64 * 1024 * 1024);

//        void shutdown(); // NOT thread safe, no simultaneous calling
//        void forceClose();
//        void forceCloseWithDelay(int64_t milliseconds);
//        void setTcpNoDelay(bool on);
//
//        // reading or not
//        void startRead();
//        void stopRead();
//
        Buffer& in_buffer();
        Buffer& out_buffer();

        virtual void connection(int32_t flag) {}
        virtual void writeComplete() {}
        virtual void highWaterMark() {}
        virtual void read(Buffer& b, const Timestamp& ts) {}

    private:
        explicit TcpSession(TcpSessionPrivate* p);

        void connectEstablished();
    };

    using STcpSession = std::shared_ptr<TcpSession>;
    using WTcpSession = std::weak_ptr<TcpSession>;

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SESSION_H_