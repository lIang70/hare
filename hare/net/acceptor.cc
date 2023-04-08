#include <hare/net/acceptor.h>

#include "hare/net/core/cycle.h"
#include "hare/net/core/event.h"
#include "hare/net/socket_op.h"
#include <hare/base/logging.h>
#include <hare/net/host_address.h>
#include <hare/net/socket.h>

#ifdef HARE__HAVE_FCNTL_H
#include <fcntl.h>
#endif // HARE__HAVE_FCNTL_H

#ifdef HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif // HARE__HAVE_NETINET_IN_H

namespace hare {
namespace net {

    class AcceptorPrivate final : public core::Event {
        friend class Acceptor;

        Socket socket_ { -1 };
        Acceptor::NewSession new_session_ {};
        int8_t family_ {};
        int16_t port_ { -1 };

#ifdef H_OS_LINUX
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        util_socket_t idle_fd_ { -1 };
#endif

        AcceptorPrivate(core::Cycle* cycle, int8_t family, int16_t port, bool reuse_port)
            : Event(cycle, socket::createNonblockingOrDie(family))
            , socket_(family, fd())
            , family_(family)
            , port_(port)
#ifdef H_OS_LINUX
            , idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
        {
            HARE_ASSERT(idle_fd_ > 0, "");
#else
        {
#endif
            socket_.setReusePort(reuse_port);
            socket_.setReuseAddr(true);
        }

        ~AcceptorPrivate() final
        {
            clearAllFlags();
            deactive();
#ifdef H_OS_LINUX
            socket::close(idle_fd_);
#endif
        }

        inline auto listen() -> bool
        {
            const HostAddress host_address(port_, false, family_ == AF_INET6);
            auto ret = socket_.bindAddress(host_address);
            if (!ret) {
                SYS_ERROR() << "Cannot bind port[" << port_ << "].";
                return ret;
            }
            ret = socket_.listen();
            if (!ret) {
                return ret;
            }
            setFlags(EVENT_READ);
            return ret;
        }

    protected:
        void eventCallBack(int32_t events, const Timestamp& receive_time) override;
    };

    void AcceptorPrivate::eventCallBack(int32_t events, const Timestamp& receive_time)
    {
        ownerCycle()->assertInCycleThread();
        if ((events & EVENT_READ) != 0) {
            HostAddress peer_addr {};
            // FIXME loop until no more ?
            auto accepted { false };
            util_socket_t conn_fd {};
            while ((conn_fd = socket_.accept(peer_addr)) >= 0) {
                LOG_TRACE() << "Accepts of " << peer_addr.toIpPort();
                if (new_session_) {
                    new_session_(conn_fd, family_, peer_addr, receive_time);
                } else {
                    socket::close(conn_fd);
                }
                accepted = true;
            }
            if (!accepted) {
                SYS_ERROR() << "Cannot accept new connect";
#ifdef H_OS_LINUX
                // Read the section named "The special problem of
                // accept()ing when you can't" in libev's doc.
                // By Marc Lehmann, author of libev.
                if (errno == EMFILE) {
                    socket::close(idle_fd_);
                    idle_fd_ = socket::accept(socket_.socket(), nullptr);
                    socket::close(idle_fd_);
                    idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
                }
#endif
            }
        } else {
            SYS_ERROR() << "Unexpected operation of acceptor.";
        }
    }

    Acceptor::Acceptor(int8_t family, int16_t port, bool reuse_port)
        : p_(new AcceptorPrivate(nullptr, family, port, reuse_port))
    {
    }

    Acceptor::~Acceptor()
    {
        delete p_;
    }

    auto Acceptor::socket() -> util_socket_t
    {
        return p_->socket_.socket();
    }

    auto Acceptor::port() -> int16_t
    {
        return p_->port_;
    }

    auto Acceptor::listen() -> bool
    {
        if (p_->ownerCycle() == nullptr) {
            SYS_ERROR() << "This acceptor[" << this << "] has not been added to any serve.";
            return false;
        }
        return p_->listen();
    }

    void Acceptor::setCycle(core::Cycle* cycle)
    {
        p_->setCycle(cycle);
    }

    void Acceptor::setNewSessionCallBack(NewSession new_session)
    {
        p_->new_session_ = std::move(new_session);
    }

} // namespace net
} // namespace hare