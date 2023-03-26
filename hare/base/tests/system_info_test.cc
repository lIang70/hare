#include <gtest/gtest.h>
#include <hare/base/util/system_info.h>

#include <iostream>

TEST(system_info_test, testGetSystemDir)
{
    auto dir = hare::util::systemDir();

    EXPECT_NE(dir.size(), 0);

    std::cout << "system dir: " << dir << std::endl;
}

TEST(system_info_test, testGetLocalIp)
{
    std::list<std::string> ips {};
    auto ret = hare::util::getLocalIp(2, ips);

    EXPECT_EQ(ret, 0);
    EXPECT_NE(ips.size(), 0);

    for (auto& ip : ips) {
        std::cout << "local ip: " << ip << std::endl;
    }
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}