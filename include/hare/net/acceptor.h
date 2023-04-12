#ifndef _HARE_NET_ACCEPTOR_H_
#define _HARE_NET_ACCEPTOR_H_

#include <hare/base/time/timestamp.h>
#include <hare/net/core/event.h>
#include <hare/net/host_address.h>
#include <hare/net/socket.h>

#include <functional>

namespace hare {
namespace core {
    class Cycle;
} // namespace core

namespace net {

    class HARE_API Acceptor : public core::Event {
    public:
        using NewSession = std::function<void(util_socket_t, HostAddress& peer_address, const Timestamp&, util_socket_t)>;

    private:
        Socket socket_;
        NewSession new_session_ {};
        int8_t family_ {};
        int16_t port_ { -1 };

#ifdef H_OS_LINUX
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        util_socket_t idle_fd_ { -1 };
#endif

    public:
        using Ptr = std::shared_ptr<Acceptor>;

        Acceptor(int8_t family, Socket::TYPE type, int16_t port, bool reuse_port = true);
        ~Acceptor() override;

        inline auto socket() const -> util_socket_t { return socket_.socket(); };
        inline auto type() const -> Socket::TYPE { return socket_.type(); };
        inline auto port() const -> int16_t { return port_; };

    protected:
        void eventCallBack(int32_t events, const Timestamp& receive_time) override;

    private:
        auto listen() -> Error;

        void setNewSessionCallBack(NewSession new_session);

        friend class HybridServe;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_ACCEPTOR_H_