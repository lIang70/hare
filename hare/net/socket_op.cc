#include "hare/net/socket_op.h"
#include "hare/base/system_check.h"
#include "hare/net/core/buffer.h"
#include <hare/base/exception.h>
#include <hare/base/logging.h>

#ifdef H_OS_WIN32
#include <winsock2.h>
#include <windows.h>
#include <io.h>
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

namespace hare {
namespace socket {

    socket_t createNonblockingOrDie(int8_t family)
    {
#ifdef H_OS_LINUX
        auto fd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
        if (fd < 0) {
            throw Exception("Cannot create non-blocking socket");
        }
#endif
        return fd;
    }

    int32_t close(socket_t fd)
    {
        return ::close(fd);
    }

    std::size_t write(socket_t fd, const void* buf, size_t size)
    {
        return ::send(fd, buf, size, MSG_NOSIGNAL);
    }

    std::size_t read(socket_t sockfd, void* buf, size_t size)
    {
        return ::recv(sockfd, buf, size, MSG_NOSIGNAL);
    }

    std::size_t getBytesReadableOnSocket(socket_t fd)
    {
#if defined(FIONREAD) && defined(H_OS_WIN32)
        auto lng = core::Buffer::MAX_READ_DEFAULT;
        if (::ioctlsocket(fd, FIONREAD, &lng) < 0)
            return -1;
        /* Can overflow, but mostly harmlessly. XXXX */
        return (int)lng;
#elif defined(FIONREAD)
        auto n = core::Buffer::MAX_READ_DEFAULT;
        if (::ioctl(fd, FIONREAD, &n) < 0)
            return -1;
        return n;
#else
        return core::Buffer::MAX_READ_DEFAULT;
#endif
    }

} // namespace socket
} // namespace hare