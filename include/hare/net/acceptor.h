/**
 * @file hare/net/acceptor.h
 * @author l1ang70 (gog_017@outlook.com)
 * @brief Describe the class associated with acceptor.h
 * @version 0.1-beta
 * @date 2023-04-16
 *
 * @copyright Copyright (c) 2023
 *
 **/

#ifndef _HARE_NET_ACCEPTOR_H_
#define _HARE_NET_ACCEPTOR_H_

#include <hare/base/io/event.h>
#include <hare/base/time/timestamp.h>
#include <hare/net/host_address.h>
#include <hare/net/socket.h>

namespace hare {
namespace net {

    HARE_CLASS_API
    class HARE_API acceptor : public io::event {
        using new_session = std::function<void(util_socket_t, host_address&, const timestamp&, acceptor*)>;

        socket socket_;
        new_session new_session_ {};
        std::int8_t family_ {};
        std::int16_t port_ { -1 };

#ifdef H_OS_LINUX
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        util_socket_t idle_fd_ { -1 };
#endif

    public:
        using ptr = std::shared_ptr<acceptor>;

        acceptor(std::int8_t _family, TYPE _type, std::int16_t _port, bool _reuse_port = true);
        ~acceptor() override;

        HARE_INLINE auto socket() const -> util_socket_t { return socket_.fd(); };
        HARE_INLINE auto type() const -> TYPE { return socket_.type(); };
        HARE_INLINE auto port() const -> std::int16_t { return port_; };

    protected:
        void event_callback(const io::event::ptr& _event, std::uint8_t _events, const timestamp& _receive_time);

    private:
        auto listen() -> error;

        HARE_INLINE void set_new_session(new_session _cb) { new_session_ = std::move(_cb); }

        friend class hybrid_serve;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_ACCEPTOR_H_