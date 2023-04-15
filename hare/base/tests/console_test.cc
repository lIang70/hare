#include <gtest/gtest.h>
#include <hare/base/io/cycle.h>
#include <hare/base/io/console.h>
#include <hare/base/logging.h>

TEST(ConsoleTest, Test1)
{
    hare::ptr<hare::io::cycle> test_cycle = std::make_shared<hare::io::cycle>(hare::io::cycle::REACTOR_TYPE_EPOLL);

    hare::io::console console;
    console.register_handle("quit", [=]{ test_cycle->exit(); });
    console.attach(test_cycle);

    test_cycle->loop();
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    hare::logger::set_level(hare::log::LEVEL_TRACE);

    return RUN_ALL_TESTS();
}
