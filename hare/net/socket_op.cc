#include "hare/net/socket_op.h"

#include <hare/base/exception.h>
#include <hare/base/io/buffer.h>
#include <hare/base/logging.h>

#if HARE__HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HARE__HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HARE__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if HARE__HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HARE__HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

namespace hare {
namespace socket_op {

    namespace detail {
#if defined(NO_ACCEPT4)
        void set_nonblock_closeonexec(int _fd)
        {
            // non-block
            auto flags = ::fcntl(_fd, F_GETFL, 0);
            flags |= O_NONBLOCK;
            auto ret = ::fcntl(_fd, F_SETFL, flags);
            // FIXME check

            // close-on-exec
            flags = ::fcntl(_fd, F_GETFD, 0);
            flags |= FD_CLOEXEC;
            ret = ::fcntl(_fd, F_SETFD, flags);
            // FIXME check

            H_UNUSED(ret);
        }
#endif
    } // namespace detail

    auto create_nonblocking_or_die(int8_t _family) -> util_socket_t
    {
#ifdef H_OS_LINUX
        auto _fd = ::socket(_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (_fd < 0) {
            throw exception("cannot create non-blocking socket");
        }
#endif
        return _fd;
    }

    auto create_dgram_or_die(int8_t _family) -> util_socket_t
    {
#ifdef H_OS_LINUX
        auto _fd = ::socket(_family, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (_fd < 0) {
            throw exception("cannot create dgram socket");
        }
#endif
        return _fd;
    }

    auto bind(int _fd, const struct sockaddr* _addr) -> error
    {
        return ::bind(_fd, _addr, sizeof(struct sockaddr_in6)) != 0 ? error(HARE_ERROR_SOCKET_BIND) : error(HARE_ERROR_SUCCESS);
    }

    auto listen(int _fd) -> error
    {
        return ::listen(_fd, SOMAXCONN) < 0 ? error(HARE_ERROR_SOCKET_LISTEN) : error(HARE_ERROR_SUCCESS);
    }

    auto connect(util_socket_t _fd, const struct sockaddr* _addr) -> error
    {
        return ::connect(_fd, _addr, sizeof(struct sockaddr)) < 0 ? error(HARE_ERROR_SOCKET_CONNECT) : error(HARE_ERROR_SUCCESS);
    }

    auto close(util_socket_t _fd) -> error
    {
        auto ret = ::close(_fd);
        if (ret < 0) {
            SYS_ERROR() << "close fd[" << _fd << "], detail:" << get_socket_error_info(_fd);
        }
        return ret < 0 ? error(HARE_ERROR_SOCKET_CLOSED) : error(HARE_ERROR_SUCCESS);
    }

    auto accept(util_socket_t _fd, struct sockaddr_in6* _addr) -> util_socket_t
    {
        auto addr_len = static_cast<socklen_t>(sizeof(*_addr));
#if defined(NO_ACCEPT4)
        auto accept_fd = ::accept(_fd, sockaddr_cast(_addr), &addr_len);
        detail::set_nonblock_closeonexec(accept_fd);
#else
        auto accept_fd = ::accept4(_fd, net::sockaddr_cast(_addr),
            &addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
        if (accept_fd < 0) {
            auto saved_errno = errno;
            switch (saved_errno) {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO: // ???
            case EPERM:
            case EMFILE: // per-process lmit of open file desctiptor ???
                // expected errors
                errno = saved_errno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                SYS_FATAL() << "unexpected error of ::accept " << saved_errno;
                break;
            default:
                SYS_FATAL() << "unknown error of ::accept " << saved_errno;
                break;
            }
        }
        return accept_fd;
    }

    auto shutdown_write(util_socket_t _fd) -> error
    {
        return ::shutdown(_fd, SHUT_WR) < 0 ? error(HARE_ERROR_SOCKET_SHUTDOWN_WRITE) : error(HARE_ERROR_SUCCESS);
    }

    auto write(util_socket_t _fd, const void* _buf, size_t size) -> int64_t
    {
        return ::write(_fd, _buf, size);
    }

    auto read(util_socket_t _fd, void* _buf, size_t size) -> int64_t
    {
        return ::read(_fd, _buf, size);
    }

    auto recvfrom(util_socket_t _fd, void* _buf, size_t size, struct sockaddr* _addr, size_t addr_len) -> int64_t
    {
        return ::recvfrom(_fd, _buf, size, 0, _addr, reinterpret_cast<socklen_t*>(&addr_len));
    }

    auto get_addr_len(int32_t _family) -> size_t
    {
        return _family == PF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
    }

    auto get_bytes_readable_on_socket(util_socket_t _fd) -> size_t
    {
#if defined(FIONREAD) && defined(H_OS_WIN32)
        auto lng = HARE_MAX_READ_DEFAULT;
        if (::ioctlsocket(target_fd, FIONREAD, &lng) < 0)
            return -1;
        /* Can overflow, but mostly harmlessly. XXXX */
        return (int)lng;
#elif defined(FIONREAD)
        auto reable_cnt = HARE_MAX_READ_DEFAULT;
        if (::ioctl(_fd, FIONREAD, &reable_cnt) < 0) {
            return (0);
        }
        return reable_cnt;
#else
        return HARE_MAX_READ_DEFAULT;
#endif
    }

    auto get_socket_error_info(util_socket_t _fd) -> std::string
    {
        std::array<char, static_cast<size_t>(HARE_SMALL_BUFFER)> error_str {};
        auto error_len = error_str.size();
        if (::getsockopt(_fd, SOL_SOCKET, SO_ERROR, error_str.data(), reinterpret_cast<socklen_t*>(&error_len)) != -1) {
            return error_str.data();
        }
        return {};
    }

    void to_ip_port(char* _buf, size_t size, const struct sockaddr* _addr)
    {
        if (_addr->sa_family == AF_INET6) {
            _buf[0] = '[';
            to_ip(_buf + 1, size - 1, _addr);
            auto end = ::strlen(_buf);
            const auto* addr6 = net::sockaddr_in6_cast(_addr);
            auto port = net::network_to_host16(addr6->sin6_port);
            HARE_ASSERT(size > end, "");
            snprintf(_buf + end, size - end, "]:%u", port);
            return;
        }
        to_ip(_buf, size, _addr);
        auto end = ::strlen(_buf);
        const auto* addr4 = net::sockaddr_in_cast(_addr);
        auto port = net::network_to_host16(addr4->sin_port);
        HARE_ASSERT(size > end, "");
        snprintf(_buf + end, size - end, ":%u", port);
    }

    void to_ip(char* _buf, size_t size, const struct sockaddr* _addr)
    {
        if (_addr->sa_family == AF_INET) {
            HARE_ASSERT(size >= INET_ADDRSTRLEN, "");
            const auto* addr4 = net::sockaddr_in_cast(_addr);
            ::inet_ntop(AF_INET, &addr4->sin_addr, _buf, static_cast<socklen_t>(size));
        } else if (_addr->sa_family == AF_INET6) {
            HARE_ASSERT(size >= INET6_ADDRSTRLEN, "");
            const auto* addr6 = net::sockaddr_in6_cast(_addr);
            ::inet_ntop(AF_INET6, &addr6->sin6_addr, _buf, static_cast<socklen_t>(size));
        }
    }

    auto from_ip_port(const char* target_ip, uint16_t port, struct sockaddr_in* _addr) -> error
    {
        _addr->sin_family = AF_INET;
        _addr->sin_port = net::host_to_network16(port);
        return ::inet_pton(AF_INET, target_ip, &_addr->sin_addr) <= 0 ? error(HARE_ERROR_SOCKET_FROM_IP) : error(HARE_ERROR_SUCCESS);
    }

    auto from_ip_port(const char* target_ip, uint16_t port, struct sockaddr_in6* _addr) -> error
    {
        _addr->sin6_family = AF_INET6;
        _addr->sin6_port = net::host_to_network16(port);
        return ::inet_pton(AF_INET6, target_ip, &_addr->sin6_addr) <= 0 ? error(HARE_ERROR_SOCKET_FROM_IP) : error(HARE_ERROR_SUCCESS);
    }

} // namespace socket_op
} // namespace hare