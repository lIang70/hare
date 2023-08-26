#include <hare/base/io/operation.h>
#include <hare/hare-config.h>

#include <cassert>

#ifdef H_OS_WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#define socklen_t int
#define close closesocket

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

#else

#include <endian.h>

#endif

#if HARE__HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HARE__HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if HARE__HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#define MAX_READ_DEFAULT 4096

namespace hare {
namespace io {

    namespace detail {

        HARE_INLINE auto SockaddrCast(const struct sockaddr_in6* _addr) -> const struct sockaddr* { return static_cast<const struct sockaddr*>(ImplicitCast<const void*>(_addr)); }
        HARE_INLINE auto SockaddrCast(struct sockaddr_in6* _addr) -> struct sockaddr* { return static_cast<struct sockaddr*>(ImplicitCast<void*>(_addr)); }
        HARE_INLINE auto SockaddrCast(const struct sockaddr_in* _addr) -> const struct sockaddr* { return static_cast<const struct sockaddr*>(ImplicitCast<const void*>(_addr)); }
        HARE_INLINE auto SockaddrCast(struct sockaddr_in* _addr) -> struct sockaddr* { return static_cast<struct sockaddr*>(ImplicitCast<void*>(_addr)); }

        static auto CreatePair(int _family, int _type, int _protocol, util_socket_t _fd[2]) -> std::int32_t
        {
            /* This socketpair does not work when localhost is down. So
             * it's really not the same thing at all. But it's close enough
             * for now, and really, when localhost is down sometimes, we
             * have other problems too.
             */
#define SET_SOCKET_ERROR(errcode) \
    do {                          \
        errno = (errcode);        \
    } while (0)
#ifdef H_OS_WIN
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

            hare::detail::FillN(&listen_addr, sizeof(listen_addr), 0);
            listen_addr.sin_family = AF_INET;
            listen_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            listen_addr.sin_port = 0;

            if (!::bind(listener, SockaddrCast(&listen_addr), static_cast<socklen_t>(sizeof(listen_addr)))) {
                goto tidy_up_and_fail;
            }
            if (::listen(listener, 1) == -1) {
                goto tidy_up_and_fail;
            }

            connector = ::socket(AF_INET, _type, 0);
            if (connector < 0) {
                goto tidy_up_and_fail;
            }

            hare::detail::FillN(&connect_addr, sizeof(connect_addr), 0);

            /* We want to find out the port number to connect to.  */
            size = sizeof(connect_addr);
            if (::getsockname(listener, SockaddrCast(&connect_addr), static_cast<socklen_t*>(&size)) == -1) {
                goto tidy_up_and_fail;
            }
            if (size != sizeof(connect_addr)) {
                goto abort_tidy_up_and_fail;
            }
            if (!::connect(connector, SockaddrCast(&connect_addr), static_cast<socklen_t>(sizeof(connect_addr)))) {
                goto tidy_up_and_fail;
            }

            size = sizeof(listen_addr);
            acceptor = ::accept(listener, SockaddrCast(&listen_addr), static_cast<socklen_t*>(&size));
            if (acceptor < 0) {
                goto tidy_up_and_fail;
            }
            if (size != sizeof(listen_addr)) {
                goto abort_tidy_up_and_fail;
            }
            /* Now check we are talking to ourself by matching port and host on the
               two sockets.	 */
            if (::getsockname(connector, SockaddrCast(&connect_addr), &size) == -1) {
                goto tidy_up_and_fail;
            }
            if (size != sizeof(connect_addr)
                || listen_addr.sin_family != connect_addr.sin_family
                || listen_addr.sin_addr.s_addr != connect_addr.sin_addr.s_addr
                || listen_addr.sin_port != connect_addr.sin_port) {
                goto abort_tidy_up_and_fail;
            }
            ::close(listener);

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
                ::close(listener);
            }
            if (connector != -1) {
                ::close(connector);
            }
            if (acceptor != -1) {
                ::close(acceptor);
            }

            SET_SOCKET_ERROR(saved_errno);
            return -1;
#undef ERR
#undef SET_SOCKET_ERROR
        }
    } // namespace detail

    auto HostToNetwork64(std::uint64_t host64) -> std::uint64_t { return htobe64(host64); }
    auto HostToNetwork32(std::uint32_t host32) -> std::uint32_t { return htobe32(host32); }
    auto HostToNetwork16(std::uint16_t host16) -> std::uint16_t { return htobe16(host16); }
    auto NetworkToHost64(std::uint64_t net64) -> std::uint64_t { return be64toh(net64); }
    auto NetworkToHost32(std::uint32_t net32) -> std::uint32_t { return be32toh(net32); }
    auto NetworkToHost16(std::uint16_t net16) -> std::uint16_t { return be16toh(net16); }

    auto SocketErrorInfo(util_socket_t _fd) -> std::string
    {
        std::array<char, HARE_SMALL_BUFFER> error_str {};
        auto error_len = error_str.size();
        if (::getsockopt(_fd, SOL_SOCKET, SO_ERROR, error_str.data(), reinterpret_cast<socklen_t*>(&error_len)) != -1) {
            return error_str.data();
        }
        return {};
    }

    auto Socketpair(std::uint8_t _family, std::int32_t _type, std::int32_t _protocol, util_socket_t _sockets[2]) -> std::int32_t
    {
#ifndef H_OS_WIN32
        return ::socketpair(_family, _type, _protocol, _sockets);
#else
        return detail::CreatePair(_family, _type, _protocol, _sockets);
#endif
    }

} // namespace io
} // namespace hare