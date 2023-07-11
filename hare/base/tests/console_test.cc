#include <gtest/gtest.h>
#include <hare/base/io/cycle.h>
#include <hare/base/io/console.h>

TEST(ConsoleTest, test1)
{
    hare::io::cycle test_cycle(hare::io::cycle::REACTOR_TYPE_EPOLL);

    auto& console = hare::io::console::instance();
    console.register_handle("quit", [&]{ test_cycle.exit(); });
    console.attach(&test_cycle);

    test_cycle.loop();
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
