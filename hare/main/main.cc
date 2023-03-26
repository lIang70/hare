#include <hare/core/stream_serve.h>

#include <hare/hare-config.h>

#ifdef HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

int main(int argc, char** argv)
{
#ifdef HARE__HAVE_EPOLL
    hare::core::StreamServe s_serve("EPOLL", AF_INET);
#elif defined(HARE__HAVE_POLL)
    hare::core::StreamServe s_serve("POLL", AF_INET);
#endif
    s_serve.init(argc, argv);

    s_serve.exec();

    return (0);
}