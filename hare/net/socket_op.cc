#include "hare/net/socket_op.h"
#include "hare/base/error.h"

#include <hare/base/exception.h>
#include <hare/base/logging.h>
#include <hare/net/buffer.h>

#ifdef H_OS_WIN32
#include <io.h>
#include <windows.h>
#include <winsock2.h>
#endif

#ifdef HARE__HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef HARE__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HARE__HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HARE__HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

namespace hare {
namespace socket {

    namespace detail {
#if defined(NO_ACCEPT4)
        void setNonBlockAndCloseOnExec(int target_fd)
        {
            // non-block
            auto flags = ::fcntl(target_fd, F_GETFL, 0);
            flags |= O_NONBLOCK;
            auto ret = ::fcntl(target_fd, F_SETFL, flags);
            // FIXME check

            // close-on-exec
            flags = ::fcntl(target_fd, F_GETFD, 0);
            flags |= FD_CLOEXEC;
            ret = ::fcntl(target_fd, F_SETFD, flags);
            // FIXME check

            H_UNUSED(ret);
        }
#endif
    } // namespace detail

    auto createNonblockingOrDie(int8_t family) -> util_socket_t
    {
#ifdef H_OS_LINUX
        auto target_fd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (target_fd < 0) {
            throw Exception("Cannot create non-blocking socket");
        }
#endif
        return target_fd;
    }

    auto createDgramOrDie(int8_t family) -> util_socket_t
    {
#ifdef H_OS_LINUX
        auto target_fd = ::socket(family, SOCK_DGRAM, 0);
        if (target_fd < 0) {
            throw Exception("Cannot create non-blocking socket");
        }
#endif
        return target_fd;
    }

    auto bind(int target_fd, const struct sockaddr* addr) -> Error
    {
        return ::bind(target_fd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6))) < 0 ? 
            Error(HARE_ERROR_SOCKET_BIND) : Error(HARE_ERROR_SUCCESS);
    }

    auto listen(int target_fd) -> Error
    {
        return ::listen(target_fd, SOMAXCONN) < 0 ?
            Error(HARE_ERROR_SOCKET_LISTEN) : Error(HARE_ERROR_SUCCESS);
    }

    auto connect(util_socket_t target_fd, const struct sockaddr* addr) -> Error
    {
        return ::connect(target_fd, addr, sizeof(struct sockaddr)) < 0 ?
            Error(HARE_ERROR_SOCKET_CONNECT) : Error(HARE_ERROR_SUCCESS);
    }


    auto close(util_socket_t target_fd) -> Error
    {
        auto ret = ::close(target_fd);
        if (ret < 0) {
            SYS_ERROR() << "Close target_fd[" << target_fd << "], detail:" << getSocketErrorInfo(target_fd);
        }
        return ret < 0 ? Error(HARE_ERROR_SOCKET_CLOSED) : Error(HARE_ERROR_SUCCESS);
    }

    auto accept(util_socket_t target_fd, struct sockaddr_in6* addr) -> util_socket_t
    {
        auto addr_len = static_cast<socklen_t>(sizeof(*addr));
#if defined(NO_ACCEPT4)
        auto accept_fd = ::accept(target_fd, sockaddr_cast(addr), &addr_len);
        detail::setNonBlockAndCloseOnExec(accept_fd);
#else
        auto accept_fd = ::accept4(target_fd, net::sockaddrCast(addr),
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

    auto shutdownWrite(util_socket_t target_fd) -> Error
    {
        return ::shutdown(target_fd, SHUT_WR) < 0 ?
            Error(HARE_ERROR_SOCKET_SHUTDOWN_WRITE) : Error(HARE_ERROR_SUCCESS);
    }

    auto write(util_socket_t target_fd, const void* buf, size_t size) -> ssize_t
    {
        return ::write(target_fd, buf, size);
    }

    auto read(util_socket_t target_fd, void* buf, size_t size) -> ssize_t
    {
        return ::read(target_fd, buf, size);
    }

    auto recvfrom(util_socket_t target_fd, void* buf, size_t size, struct sockaddr* addr, size_t addr_len) -> ssize_t
    {
        return ::recvfrom(target_fd, buf, size, 0, addr, reinterpret_cast<socklen_t*>(&addr_len));
    }

    auto getBytesReadableOnSocket(util_socket_t target_fd) -> std::size_t
    {
#if defined(FIONREAD) && defined(H_OS_WIN32)
        auto lng = net::Buffer::MAX_READ_DEFAULT;
        if (::ioctlsocket(target_fd, FIONREAD, &lng) < 0)
            return -1;
        /* Can overflow, but mostly harmlessly. XXXX */
        return (int)lng;
#elif defined(FIONREAD)
        auto reable_cnt = net::Buffer::MAX_READ_DEFAULT;
        if (::ioctl(target_fd, FIONREAD, &reable_cnt) < 0) {
            return (0);
        }
        return reable_cnt;
#else
        return net::Buffer::MAX_READ_DEFAULT;
#endif
    }

    auto getSocketErrorInfo(util_socket_t target_fd) -> std::string
    {
        char error_str[HARE_SMALL_FIXED_SIZE * HARE_SMALL_FIXED_SIZE];
        auto error_len = static_cast<int32_t>(sizeof(error_str));
        if (::getsockopt(target_fd, SOL_SOCKET, SO_ERROR, error_str, reinterpret_cast<socklen_t*>(&error_len)) != -1) {
            return error_str;
        }
        return {};
    }

    void toIpPort(char* buf, size_t size, const struct sockaddr* addr)
    {
        if (addr->sa_family == AF_INET6) {
            buf[0] = '[';
            toIp(buf + 1, size - 1, addr);
            auto end = ::strlen(buf);
            const auto* addr6 = net::sockaddrIn6Cast(addr);
            auto port = net::networkToHost16(addr6->sin6_port);
            HARE_ASSERT(size > end, "");
            snprintf(buf + end, size - end, "]:%u", port);
            return;
        }
        toIp(buf, size, addr);
        auto end = ::strlen(buf);
        const auto* addr4 = net::sockaddrInCast(addr);
        auto port = net::networkToHost16(addr4->sin_port);
        HARE_ASSERT(size > end, "");
        snprintf(buf + end, size - end, ":%u", port);
    }

    void toIp(char* buf, size_t size, const struct sockaddr* addr)
    {
        if (addr->sa_family == AF_INET) {
            HARE_ASSERT(size >= INET_ADDRSTRLEN, "");
            const auto* addr4 = net::sockaddrInCast(addr);
            ::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
        } else if (addr->sa_family == AF_INET6) {
            HARE_ASSERT(size >= INET6_ADDRSTRLEN, "");
            const auto* addr6 = net::sockaddrIn6Cast(addr);
            ::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
        }
    }

    auto fromIpPort(const char* target_ip, uint16_t port, struct sockaddr_in* addr) -> Error
    {
        addr->sin_family = AF_INET;
        addr->sin_port = net::hostToNetwork16(port);
        return ::inet_pton(AF_INET, target_ip, &addr->sin_addr) <= 0 ?
            Error(HARE_ERROR_SOCKET_FROM_IP) : Error(HARE_ERROR_SUCCESS);
    }

    auto fromIpPort(const char* target_ip, uint16_t port, struct sockaddr_in6* addr) -> Error
    {
        addr->sin6_family = AF_INET6;
        addr->sin6_port = net::hostToNetwork16(port);
        return ::inet_pton(AF_INET6, target_ip, &addr->sin6_addr) <= 0 ?
            Error(HARE_ERROR_SOCKET_FROM_IP) : Error(HARE_ERROR_SUCCESS);
    }

} // namespace socket
} // namespace hare