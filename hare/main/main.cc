#include "main/manage/input.h"

#include <hare/hare-config.h>
#include <hare/core/stream_serve.h>

#include <cstdio>

static void printLogo()
{
    ::fprintf(stdout, "====== Welcome to HARE ======\n");
    ::fflush(stdout);
}

auto main(int argc, char** argv) -> int
{
    hare::manage::Input::instance().init(argc, argv);

#ifdef HARE__HAVE_EPOLL
    hare::core::StreamServe::Ptr s_serve = std::make_shared<hare::core::StreamServe>("EPOLL");
#elif defined(HARE__HAVE_POLL)
    hare::core::StreamServe::Ptr s_serve = std::make_shared<hare::core::StreamServe>("POLL");
#endif

    auto ret = hare::manage::Input::instance().initServe(s_serve);
    if (!ret) {
        return (-1);
    }

    printLogo();

    s_serve->exec();

    return (0);
}