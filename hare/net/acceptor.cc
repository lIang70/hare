#include <hare/net/acceptor.h>

#include "hare/net/core/cycle.h"
#include "hare/net/host_address.h"
#include "hare/net/socket.h"
#include "hare/net/socket_op.h"
#include <hare/base/logging.h>

#ifdef HARE__HAVE_FCNTL_H
#include <fcntl.h>
#endif // HARE__HAVE_FCNTL_H

#ifdef HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif // HARE__HAVE_NETINET_IN_H

namespace hare {
namespace net {

    Acceptor::Acceptor(int8_t family, Socket::TYPE type, int16_t port, bool reuse_port)
        : Event(nullptr, type == Socket::TYPE_TCP ? 
                         socket::createNonblockingOrDie(family) : (type == Socket::TYPE_UDP ? 
                                                                   socket::createDgramOrDie(family) : -1))
        , socket_(family, type, fd())
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

    Acceptor::~Acceptor()
    {
        clearAllFlags();
        deactive();
#ifdef H_OS_LINUX
        socket::close(idle_fd_);
#endif
    }

    void Acceptor::eventCallBack(int32_t events, const Timestamp& receive_time)
    {
        if ((events & EVENT_READ) != 0) {
            HostAddress peer_addr {};
            auto local_addr = HostAddress::localAddress(socket());
            
            // FIXME loop until no more ?
            auto accepted { false };
            util_socket_t conn_fd {};
            
            switch (type()) {
                case Socket::TYPE_TCP: 
                if ((conn_fd = socket_.accept(peer_addr)) >= 0) {
                    LOG_TRACE() << "Accepts of " << peer_addr.toIpPort();
                    if (new_session_) {
                        new_session_(conn_fd, peer_addr, receive_time, socket());
                    } else {
                        socket::close(conn_fd);
                    }
                    accepted = true;
                }
                    break;
                case Socket::TYPE_UDP:
                if ((conn_fd = socket_.accept(peer_addr, &local_addr)) >= 0) {
                    LOG_TRACE() << "Accepts of " << peer_addr.toIpPort();
                    if (new_session_) {
                        new_session_(conn_fd, peer_addr, receive_time, socket());
                    } else {
                        socket::close(conn_fd);
                    }
                    accepted = true;
                }
                    break;
                case Socket::TYPE_INVALID:
                default:
                    break;
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

    auto Acceptor::listen() -> Error
    {
        if (ownerCycle() == nullptr) {
            SYS_ERROR() << "This acceptor[" << this << "] has not been added to any cycle.";
            return Error(HARE_ERROR_ACCEPTOR_ACTIVED);
        }
        const HostAddress host_address(port_, false, family_ == AF_INET6);
        auto ret = socket_.bindAddress(host_address);
        if (!ret) {
            SYS_ERROR() << "Cannot bind port[" << port_ << "].";
            return ret;
        }
        if (type() == Socket::TYPE_TCP) {
            ret = socket_.listen();
            if (!ret) {
                return ret;
            }
        }
        setFlags(EVENT_READ);
        return ret;
    }

    void Acceptor::setNewSessionCallBack(NewSession new_session)
    {
        new_session_ = std::move(new_session);
    }

} // namespace net
} // namespace hare