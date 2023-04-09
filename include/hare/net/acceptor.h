#ifndef _HARE_NET_ACCEPTOR_H_
#define _HARE_NET_ACCEPTOR_H_

#include <hare/base/time/timestamp.h>
#include <hare/net/host_address.h>

#include <functional>
#include <memory>

namespace hare {
namespace core {
    class Cycle;
} // namespace core

namespace net {

    class AcceptorPrivate;
    class HARE_API Acceptor {
        friend class TcpServe;

        AcceptorPrivate* p_ { nullptr };

    public:
        using Ptr = std::shared_ptr<Acceptor>;
        using NewSession = std::function<void(util_socket_t, int8_t, const HostAddress& conn_address, const Timestamp&, util_socket_t)>;

        Acceptor(int8_t family, int16_t port, bool reuse_port = true);
        ~Acceptor();

        auto socket() -> util_socket_t;

        auto port() -> int16_t;

    private:
        auto listen() -> bool;

        void setCycle(core::Cycle* cycle);

        void setNewSessionCallBack(NewSession new_session);
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_ACCEPTOR_H_