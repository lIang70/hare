#ifndef _HARE_NET_ACCEPTOR_H_
#define _HARE_NET_ACCEPTOR_H_

#include <hare/base/time/timestamp.h>
#include <hare/base/io/event.h>
#include <hare/net/host_address.h>
#include <hare/net/socket.h>

namespace hare {
namespace net {

    class HARE_API acceptor : public io::event {
        using new_session = std::function<void(util_socket_t, host_address&, const timestamp&, util_socket_t)>;

        socket socket_;
        new_session new_session_ {};
        int8_t family_ {};
        int16_t port_ { -1 };

#ifdef H_OS_LINUX
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        util_socket_t idle_fd_ { -1 };
#endif

    public:
        using ptr = std::shared_ptr<acceptor>;

        acceptor(int8_t _family, TYPE _type, int16_t _port, bool _reuse_port = true);
        ~acceptor() override;

        inline auto socket() const -> util_socket_t { return socket_.fd(); };
        inline auto type() const -> TYPE { return socket_.type(); };
        inline auto port() const -> int16_t { return port_; };

    protected:
        void event_callback(const io::event::ptr& _event, uint8_t _events, const timestamp& _receive_time);

    private:
        auto listen() -> error;
        void set_new_session(new_session _cb);

        friend class hybrid_serve;
    };

} // namespace net
} // namespace hare

#endif // !_HARE_NET_ACCEPTOR_H_