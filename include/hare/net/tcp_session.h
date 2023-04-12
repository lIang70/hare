#ifndef _HARE_NET_TCP_SESSION_H_
#define _HARE_NET_TCP_SESSION_H_

#include <hare/net/session.h>

namespace hare {
namespace net {

    class HARE_API TcpSession : public Session
                              , public std::enable_shared_from_this<TcpSession> {
    public:
        using Ptr = std::shared_ptr<TcpSession>;

        ~TcpSession() override;

        auto send(const uint8_t* bytes, std::size_t length) -> bool override;

        auto setTcpNoDelay(bool tcp_on) -> Error;

    protected:
        TcpSession(core::Cycle* cycle,
            HostAddress local_addr,
            std::string name, int8_t family, util_socket_t target_fd,
            HostAddress peer_addr);

        void handleRead(const Timestamp& receive_time) override;
        void handleWrite() override;

        void connection(int32_t flag) override { }
        void writeComplete() override { }
        void highWaterMark() override { }
        void read(Buffer& buffer, const Timestamp& time) override { }

        friend class HybridServe;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_TCP_SESSION_H_