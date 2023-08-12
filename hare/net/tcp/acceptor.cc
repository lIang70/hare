#include "hare/base/fwd-inl.h"
#include "hare/base/io/reactor.h"
#include "hare/base/io/socket_op-inl.h"
#include <hare/hare-config.h>
#include <hare/net/socket.h>
#include <hare/net/tcp/acceptor.h>

#if HARE__HAVE_FCNTL_H
#include <fcntl.h>
#endif // HARE__HAVE_FCNTL_H

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif // HARE__HAVE_NETINET_IN_H

#if defined(H_OS_WIN)
#include <WinSock2.h>
#endif // H_OS_WIN

namespace hare {
namespace net {

    HARE_IMPL_DEFAULT(
        Acceptor,
        Socket socket;
        Acceptor::NewSessionCallback new_session {};
        std::uint16_t port {};
        
        AcceptorImpl(std::uint8_t _family, util_socket_t _fd, std::uint16_t _port)
            : socket(_family, TYPE_TCP, _fd)
            , port(_port) {}
    )

    Acceptor::Acceptor(std::uint8_t _family, std::uint16_t _port, bool _reuse_port)
        : io::Event(socket_op::CreateNonblockingOrDie(_family),
            std::bind(&Acceptor::EventCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            io::EVENT_PERSIST,
            0)
        , impl_(new AcceptorImpl { _family, fd(), _port })
#ifdef H_OS_LINUX
        , idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
    {
        assert(idle_fd_ > 0);
#else
    {
#endif
        d_ptr(impl_)->socket.SetReusePort(_reuse_port);
        d_ptr(impl_)->socket.SetReuseAddr(true);
    }

    Acceptor::~Acceptor()
    {
        if (!TiedObject().expired()) {
            Tie(nullptr);
            Deactivate();
        }
#ifdef H_OS_LINUX
        socket_op::Close(idle_fd_);
#endif
        delete impl_;
    }

    auto Acceptor::Socket() const -> util_socket_t
    {
        return d_ptr(impl_)->socket.fd();
    }

    auto Acceptor::Port() const -> std::uint16_t
    {
        return d_ptr(impl_)->port;
    }

    auto Acceptor::Family() const -> std::uint8_t
    {
        return d_ptr(impl_)->socket.family();
    }

    void Acceptor::EventCallback(const Ptr<io::Event>& _event, std::uint8_t _events, const Timestamp& _receive_time)
    {
        assert(this->shared_from_this() == _event);
        if (CHECK_EVENT(_events, io::EVENT_READ) == 0) {
            MSG_ERROR("unexpected operation of acceptor.");
            return;
        }

        HostAddress peer_addr {};
        auto local_addr = HostAddress::LocalAddress(Socket());

        auto accepted { false };
        util_socket_t conn_fd {};

        /// FIXME: loop until no more ?
        while ((conn_fd = d_ptr(impl_)->socket.Accept(peer_addr)) >= 0) {
            MSG_TRACE("accepts of tcp[{}].", peer_addr.ToIpPort());
            if (d_ptr(impl_)->new_session) {
                d_ptr(impl_)->new_session(conn_fd, peer_addr, _receive_time, this);
            } else {
                socket_op::Close(conn_fd);
            }
            accepted = true;
        }

        if (!accepted) {
            MSG_ERROR("cannot accept new connect.");
#ifdef H_OS_LINUX
            // Read the section named "The special problem of
            // accept()ing when you can't" in libev's doc.
            // By Marc Lehmann, author of libev.
            if (errno == EMFILE) {
                socket_op::Close(idle_fd_);
                idle_fd_ = socket_op::Accept(d_ptr(impl_)->socket.fd(), nullptr, 0);
                socket_op::Close(idle_fd_);
                idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
            }
#endif
        }
    }

    auto Acceptor::Listen() -> Error
    {
        if (cycle() == nullptr) {
            MSG_ERROR("this acceptor[{}] has not been added to any cycle.", (void*)this);
            return Error(ERROR_ACCEPTOR_ACTIVED);
        }
        const HostAddress address(d_ptr(impl_)->port, false, d_ptr(impl_)->socket.family() == AF_INET6);
        auto ret = d_ptr(impl_)->socket.BindAddress(address);
        if (!ret) {
            return ret;
        }
        ret = d_ptr(impl_)->socket.Listen();
        if (!ret) {
            return ret;
        }
        Tie(shared_from_this());
        EnableRead();
        return ret;
    }

    void Acceptor::SetNewSession(NewSessionCallback _cb)
    {
        d_ptr(impl_)->new_session = std::move(_cb);
    }

} // namespace net
} // namespace hare
