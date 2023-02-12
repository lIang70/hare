#include "hare/net/socket_op.h"
#include "hare/base/system_check.h"
#include <hare/base/exception.h>
#include <hare/base/logging.h>

#ifdef H_OS_WIN32
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
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

    ssize_t write(socket_t fd, const void* buf, size_t size)
    {
        return ::send(fd, buf, size, MSG_NOSIGNAL);
    }

    ssize_t read(socket_t sockfd, void* buf, size_t size)
    {
        return ::recv(sockfd, buf, size, MSG_NOSIGNAL);
    }

} // namespace socket
} // namespace hare