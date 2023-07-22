#include "hare/base/fwd-inl.h"
#include <hare/base/exception.h>
#include <hare/base/io/socket_op.h>
#include <hare/hare-config.h>

#include <cassert>

#ifdef H_OS_WIN32
#include <WinSock2.h>

#if BYTE_ORDER == LITTLE_ENDIAN

#define htobe16(x) htons(x)
#define htole16(x) (x)
#define be16toh(x) ntohs(x)
#define le16toh(x) (x)

#define htobe32(x) htonl(x)
#define htole32(x) (x)
#define be32toh(x) ntohl(x)
#define le32toh(x) (x)

#define htobe64(x) htonll(x)
#define htole64(x) (x)
#define be64toh(x) ntohll(x)
#define le64toh(x) (x)

#elif BYTE_ORDER == BIG_ENDIAN

#define htobe16(x) (x)
#define htole16(x) __builtin_bswap16(x)
#define be16toh(x) (x)
#define le16toh(x) __builtin_bswap16(x)

#define htobe32(x) (x)
#define htole32(x) __builtin_bswap32(x)
#define be32toh(x) (x)
#define le32toh(x) __builtin_bswap32(x)

#define htobe64(x) (x)
#define htole64(x) __builtin_bswap64(x)
#define be64toh(x) (x)
#define le64toh(x) __builtin_bswap64(x)

#endif

#define socklen_t std::uint32_t

#else
#include <endian.h>
#endif

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

#define MAX_READ_DEFAULT 4096

#define SET_SOCKET_ERROR(errcode) \
    do {                          \
        errno = (errcode);        \
    } while (0)

namespace hare {
namespace socket_op {

    namespace detail {
#if defined(NO_ACCEPT4)
        static void set_nonblock_closeonexec(util_socket_t _fd)
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

            ignore_unused(ret);
        }
#endif

        static auto create_pair(int _family, int _type, int _protocol, util_socket_t _fd[2]) -> std::int32_t
        {
            /* This socketpair does not work when localhost is down. So
             * it's really not the same thing at all. But it's close enough
             * for now, and really, when localhost is down sometimes, we
             * have other problems too.
             */
#ifdef _WIN32
#define ERR(e) WSA##e
#else
#define ERR(e) e
#endif
            util_socket_t listener = -1;
            util_socket_t connector = -1;
            util_socket_t acceptor = -1;
            struct sockaddr_in listen_addr { };
            struct sockaddr_in connect_addr { };
            socklen_t size {};
            std::int32_t saved_errno = -1;

            auto family_test = _family != AF_INET;
#ifdef AF_UNIX
            family_test = family_test && (_type != AF_UNIX);
#endif
            if (_protocol || family_test) {
                SET_SOCKET_ERROR(ERR(EAFNOSUPPORT));
                return -1;
            }

            if (!_fd) {
                SET_SOCKET_ERROR(ERR(EINVAL));
                return -1;
            }

            listener = ::socket(AF_INET, _type, 0);
            if (listener < 0) {
                return -1;
            }

            hare::detail::fill_n(&listen_addr, sizeof(listen_addr), 0);
            listen_addr.sin_family = AF_INET;
            listen_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            listen_addr.sin_port = 0;

            if (!socket_op::bind(listener, (struct sockaddr*)&listen_addr, sizeof(listen_addr))) {
                goto tidy_up_and_fail;
            }
            if (::listen(listener, 1) == -1) {
                goto tidy_up_and_fail;
            }

            connector = ::socket(AF_INET, _type, 0);
            if (connector < 0) {
                goto tidy_up_and_fail;
            }

            hare::detail::fill_n(&connect_addr, sizeof(connect_addr), 0);

            /* We want to find out the port number to connect to.  */
            size = sizeof(connect_addr);
            if (::getsockname(listener, (struct sockaddr*)&connect_addr, &size) == -1) {
                goto tidy_up_and_fail;
            }
            if (size != sizeof(connect_addr)) {
                goto abort_tidy_up_and_fail;
            }
            if (::connect(connector, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) == -1) {
                goto tidy_up_and_fail;
            }

            size = sizeof(listen_addr);
            acceptor = ::accept(listener, (struct sockaddr*)&listen_addr, &size);
            if (acceptor < 0) {
                goto tidy_up_and_fail;
            }
            if (size != sizeof(listen_addr)) {
                goto abort_tidy_up_and_fail;
            }
            /* Now check we are talking to ourself by matching port and host on the
               two sockets.	 */
            if (::getsockname(connector, (struct sockaddr*)&connect_addr, &size) == -1) {
                goto tidy_up_and_fail;
            }
            if (size != sizeof(connect_addr)
                || listen_addr.sin_family != connect_addr.sin_family
                || listen_addr.sin_addr.s_addr != connect_addr.sin_addr.s_addr
                || listen_addr.sin_port != connect_addr.sin_port) {
                goto abort_tidy_up_and_fail;
            }
            socket_op::close(listener);

            _fd[0] = connector;
            _fd[1] = acceptor;

            return 0;

        abort_tidy_up_and_fail:
            saved_errno = ERR(ECONNABORTED);
        tidy_up_and_fail:
            if (saved_errno < 0) {
                saved_errno = errno;
            }
            if (listener != -1) {
                socket_op::close(listener);
            }
            if (connector != -1) {
                socket_op::close(connector);
            }
            if (acceptor != -1) {
                socket_op::close(acceptor);
            }

            SET_SOCKET_ERROR(saved_errno);
            return -1;
#undef ERR
        }
    } // namespace detail

    auto host_to_network64(std::uint64_t host64) -> std::uint64_t { return htobe64(host64); }
    auto host_to_network32(std::uint32_t host32) -> std::uint32_t { return htobe32(host32); }
    auto host_to_network16(std::uint16_t host16) -> std::uint16_t { return htobe16(host16); }
    auto network_to_host64(std::uint64_t net64) -> std::uint64_t { return be64toh(net64); }
    auto network_to_host32(std::uint32_t net32) -> std::uint32_t { return be32toh(net32); }
    auto network_to_host16(std::uint16_t net16) -> std::uint16_t { return be16toh(net16); }

    auto create_nonblocking_or_die(std::uint8_t _family) -> util_socket_t
    {
#ifndef H_OS_WIN
        auto _fd = ::socket(_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (_fd < 0) {
            throw exception("cannot create non-blocking socket");
        }
#else
#endif
        return _fd;
    }

    auto create_dgram_or_die(std::uint8_t _family) -> util_socket_t
    {
#ifndef H_OS_WIN
        auto _fd = ::socket(_family, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (_fd < 0) {
            throw exception("cannot create dgram socket");
        }
#else
#endif
        return _fd;
    }

    auto close(util_socket_t _fd) -> std::int32_t
    {
#ifndef H_OS_WIN
        auto ret = ::close(_fd);
        if (ret < 0) {
            MSG_ERROR("close fd[{}], detail:{}.", _fd, get_socket_error_info(_fd));
        }
#else
#endif
        return ret;
    }

    auto write(util_socket_t _fd, const void* _buf, std::size_t size) -> std::int64_t
    {
#ifndef H_OS_WIN
        return ::write(_fd, _buf, size);
#else
#endif
    }

    auto read(util_socket_t _fd, void* _buf, std::size_t size) -> std::int64_t
    {
#ifndef H_OS_WIN
        return ::read(_fd, _buf, size);
#else
#endif
    }

    auto sendto(util_socket_t _fd, void* _buf, std::size_t _size, struct sockaddr* _addr, std::size_t _addr_len) -> std::int64_t
    {
#ifndef H_OS_WIN
        return ::sendto(_fd, _buf, _size, 0, _addr, _addr_len);
#else
#endif
    }

    auto recvfrom(util_socket_t _fd, void* _buf, std::size_t size, struct sockaddr* _addr, std::size_t addr_len) -> std::int64_t
    {
#ifndef H_OS_WIN
        return ::recvfrom(_fd, _buf, size, 0, _addr, reinterpret_cast<socklen_t*>(&addr_len));
#else
#endif
    }

    auto bind(util_socket_t _fd, const struct sockaddr* _addr, std::size_t _addr_len) -> bool
    {
#ifndef H_OS_WIN
        return ::bind(_fd, _addr, _addr_len) != 0;
#else
#endif
    }

    auto listen(util_socket_t _fd) -> bool
    {
#ifndef H_OS_WIN
        return ::listen(_fd, SOMAXCONN) < 0;
#else
#endif
    }

    auto connect(util_socket_t _fd, const struct sockaddr* _addr, std::size_t _addr_len) -> bool
    {
#ifndef H_OS_WIN
        return ::connect(_fd, _addr, _addr_len) < 0;
#else
#endif
    }

    auto accept(util_socket_t _fd, struct sockaddr* _addr, std::size_t _addr_len) -> util_socket_t
    {
#if defined(NO_ACCEPT4)
        auto accept_fd = ::accept(_fd, _addr, (socklen_t*)&_addr_len);
        detail::set_nonblock_closeonexec(accept_fd);
#else
        auto accept_fd = ::accept4(_fd, _addr,
            (socklen_t*)&_addr_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
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
                MSG_FATAL("unexpected error of ::accept {}.", saved_errno);
                break;
            default:
                MSG_FATAL("unknown error of ::accept {}.", saved_errno);
                break;
            }
        }
        return accept_fd;
    }

    auto get_addr_len(std::uint8_t _family) -> std::size_t
    {
        return _family == PF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
    }

    auto get_bytes_readable_on_socket(util_socket_t _fd) -> std::int64_t
    {
#if defined(FIONREAD) && defined(H_OS_WIN32)
        auto lng = MAX_READ_DEFAULT;
        if (::ioctlsocket(target_fd, FIONREAD, &lng) < 0) {
            return (-1);
        }
        /* Can overflow, but mostly harmlessly. XXXX */
        return lng;
#elif defined(FIONREAD)
        auto reable_cnt = MAX_READ_DEFAULT;
        if (::ioctl(_fd, FIONREAD, &reable_cnt) < 0) {
            return (0);
        }
        return reable_cnt;
#else
        return MAX_READ_DEFAULT;
#endif
    }

    auto get_socket_error_info(util_socket_t _fd) -> std::string
    {
        std::array<char, static_cast<std::size_t>(HARE_SMALL_BUFFER)> error_str {};
        auto error_len = error_str.size();
        if (::getsockopt(_fd, SOL_SOCKET, SO_ERROR, error_str.data(), reinterpret_cast<socklen_t*>(&error_len)) != -1) {
            return error_str.data();
        }
        return {};
    }

    void to_ip_port(char* _buf, std::size_t size, const struct sockaddr* _addr)
    {
        if (_addr->sa_family == AF_INET6) {
            _buf[0] = '[';
            to_ip(_buf + 1, size - 1, _addr);
            auto end = ::strlen(_buf);
            const auto* addr6 = sockaddr_in6_cast(_addr);
            auto port = network_to_host16(addr6->sin6_port);
            assert(size > end);
            ignore_unused(::snprintf(_buf + end, size - end, "]:%u", port));
            return;
        }
        to_ip(_buf, size, _addr);
        auto end = ::strlen(_buf);
        const auto* addr4 = sockaddr_in_cast(_addr);
        auto port = network_to_host16(addr4->sin_port);
        assert(size > end);
        ignore_unused(::snprintf(_buf + end, size - end, ":%u", port));
    }

    void to_ip(char* _buf, std::size_t size, const struct sockaddr* _addr)
    {
        if (_addr->sa_family == AF_INET) {
            assert(size >= INET_ADDRSTRLEN);
            const auto* addr4 = sockaddr_in_cast(_addr);
            ::inet_ntop(AF_INET, &addr4->sin_addr, _buf, static_cast<socklen_t>(size));
        } else if (_addr->sa_family == AF_INET6) {
            assert(size >= INET6_ADDRSTRLEN);
            const auto* addr6 = sockaddr_in6_cast(_addr);
            ::inet_ntop(AF_INET6, &addr6->sin6_addr, _buf, static_cast<socklen_t>(size));
        }
    }

    auto from_ip_port(const char* target_ip, uint16_t port, struct sockaddr_in* _addr) -> bool
    {
        _addr->sin_family = AF_INET;
        _addr->sin_port = host_to_network16(port);
        return ::inet_pton(AF_INET, target_ip, &_addr->sin_addr) <= 0;
    }

    auto from_ip_port(const char* target_ip, uint16_t port, struct sockaddr_in6* _addr) -> bool
    {
        _addr->sin6_family = AF_INET6;
        _addr->sin6_port = host_to_network16(port);
        return ::inet_pton(AF_INET6, target_ip, &_addr->sin6_addr) <= 0;
    }

    auto socketpair(std::uint8_t _family, std::int32_t _type, std::int32_t _protocol, util_socket_t _sockets[2]) -> std::int32_t
    {
#ifndef M_OS_WIN32
        return ::socketpair(_family, _type, _protocol, _sockets);
#else
        return detail::create_pair(_family, _type, _protocol, _sockets);
#endif
    }

} // namespace socket_op
} // namespace hare