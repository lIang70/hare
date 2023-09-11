#include <gtest/gtest.h>
#include <hare/base/util/system.h>

TEST(SystemTest, testCpuUsage) {
  auto cpu_usage = hare::util::CpuUsage(hare::util::Pid());
  std::cout << "cpu_usage:" << cpu_usage << std::endl;
  GTEST_ASSERT_LT(std::abs(cpu_usage - 0.0), 1e-5);
}

auto main(int argc, char** argv) -> int {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
