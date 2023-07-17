#include <gtest/gtest.h>
#include <hare/log/details/msg.h>

TEST(FormatMsgTest, test1)
{
    std::string logger = "default";
    hare::timezone tz { 8 * 3600, "Z" };

    hare::log::details::msg test_msg
    {
        &logger, &tz, hare::log::LEVEL_TRACE, { __FILE__, __LINE__, __func__ }
    };

    hare::log::details::msg_buffer_t fotmatted {};

    fmt::format_to(std::back_inserter(test_msg.raw_), "test\0");

    hare::log::details::format_msg(test_msg, fotmatted);

    std::cout << fotmatted.data() << std::endl;
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}