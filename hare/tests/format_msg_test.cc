#include <gtest/gtest.h>
#include <hare/log/details/msg.h>

TEST(FormatMsgTest, test1)
{
    std::string logger = "default";
    hare::Timezone tz { 8 * 3600, "Z" };
    hare::log::SourceLoc loc { __FILE__, __LINE__, __func__ };

    hare::log::detail::Msg test_msg
    {
        &logger, &tz, hare::log::LEVEL_TRACE, loc
    };

    hare::log::detail::msg_buffer_t fotmatted {};

    fmt::format_to(std::back_inserter(test_msg.raw_), "test\0");

    hare::log::detail::FormatMsg(test_msg, fotmatted);

    std::cout << fotmatted.data() << std::endl;
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
