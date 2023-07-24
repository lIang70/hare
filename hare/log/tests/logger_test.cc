#include <gtest/gtest.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/system.h>
#include <hare/log/backends/file_backend.h>
#include <hare/log/registry.h>

TEST(LoggerTest, bench)
{
    using hare::log::backend;
    using hare::log::file_backend_st;
    using hare::log::detail::rotate_file;

    constexpr std::uint64_t file_size = static_cast<std::uint64_t>(64) * 1024 * 1024;

    auto tmp = hare::util::system_dir();
    std::vector<hare::ptr<backend>> backends {
        std::make_shared<file_backend_st<rotate_file<file_size>>>(tmp + "/logger_test")
    };

    auto test_logger = hare::log::registry::create("logger_test", backends.begin(), backends.end());

    constexpr std::int32_t log_size = 250000;

    auto start { hare::timestamp::now() };
    for (auto i = 0; i < log_size; ++i) {
        LOG_INFO(test_logger, "test string {}", "AAAAAAAAAAAAAAAAAAAA");
        LOG_INFO(test_logger, "test double {}", 4.32424);
        LOG_INFO(test_logger, "test int {}", 43452342);
        LOG_INFO(test_logger, "test mix {} {} {}", "AAAAAAAAAAAAAAAAAAAA", 4.32424, 43452342);
    }
    test_logger->flush();
    auto end { hare::timestamp::now() };
    auto gap = hare::timestamp::difference(end, start);
    fmt::print("test logger speed: {} s/{}p" HARE_EOL, gap, log_size * 4);
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
