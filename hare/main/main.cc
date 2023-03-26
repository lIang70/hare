#include <hare/core/stream_serve.h>

#include <hare/hare-config.h>

#ifdef HARE__HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

auto main(int argc, char** argv) -> int
{
#ifdef HARE__HAVE_EPOLL
    hare::core::StreamServe::Ptr s_serve = std::make_shared<hare::core::StreamServe>("EPOLL", AF_INET);
#elif defined(HARE__HAVE_POLL)
    hare::core::StreamServe::Ptr s_serve = std::make_shared<hare::core::StreamServe>("POLL", AF_INET);
#endif
    s_serve->init(argc, argv);

    s_serve->exec();

    return (0);
}