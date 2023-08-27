#include <gtest/gtest.h>
#include <hare/base/time/timestamp.h>
#include <hare/base/util/system.h>
#include <hare/log/backends/file_backend.h>
#include <hare/log/registry.h>

TEST(AsyncLoggerTest, bench)
{
    using hare::log::Backend;
    using hare::log::FileBackendMT;
    using hare::log::detail::RotateFileBySize;

    constexpr std::uint64_t file_size = static_cast<std::uint64_t>(64) * 1024 * 1024;

    auto tmp = hare::util::SystemDir();
    std::vector<hare::Ptr<Backend>> backends {
        std::make_shared<FileBackendMT<RotateFileBySize<file_size>>>(tmp + "/async_logger_test")
    };

    auto test_logger = 
        hare::log::Registry::Create("async_logger_test", backends.begin(), backends.end(), 
            5000, std::thread::hardware_concurrency());
        
    test_logger->set_policy(hare::log::Policy::BLOCK_RETRY);

    constexpr std::int32_t log_size = 250000;

    auto start { hare::Timestamp::Now() };
    for (auto i = 0; i < log_size; ++i) {
        LOG_INFO(test_logger, "test string {}", "AAAAAAAAAAAAAAAAAAAA");
        LOG_INFO(test_logger, "test double {}", 4.32424);
        LOG_INFO(test_logger, "test int {}", 43452342);
        LOG_INFO(test_logger, "test mix {} {} {}", "AAAAAAAAAAAAAAAAAAAA", 4.32424, 43452342);
    }
    test_logger->Flush();
    auto end { hare::Timestamp::Now() };
    auto gap = hare::Timestamp::Difference(end, start);
    fmt::print("test logger speed: {} s/{}p" HARE_EOL, gap, log_size * 4);
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
