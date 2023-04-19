#include "main/manage/input.h"

#include <hare/hare-config.h>
#include <hare/core/stream_serve.h>

#include <cstdio>

static void print_logo()
{
    ::fprintf(stdout, "====== Welcome to HARE ======\n");
    ::fflush(stdout);
}

auto main(int argc, char** argv) -> int32_t
{
    manage::input::instance().init(argc, argv);

#ifdef HARE__HAVE_EPOLL
    hare::core::stream_serve stream_serve(hare::io::cycle::REACTOR_TYPE_EPOLL);
#elif defined(HARE__HAVE_POLL)
    hare::core::stream_serve s_serve(hare::io::cycle::REACTOR_TYPE_POLL);
#endif

    if (!manage::input::instance().init_serve(stream_serve)) {
        return (-1);
    }

    print_logo();

    stream_serve.run();

    return (0);
}