#include <gtest/gtest.h>
#include <hare/net/buffer.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>


TEST(BufferTest, test)
{
    using hare::net::buffer;
    buffer test_buffer {};

    constexpr std::size_t big_buffer_size { 0xc000 };
    constexpr std::size_t mid_buffer_size { 0x8000 };
    constexpr std::size_t small_buffer_size { 0x4000 };
    auto* tmp_buffer = new char[big_buffer_size];
    hare::detail::fill_n(tmp_buffer, big_buffer_size, 'z');

    for (auto i = 0; i < 16; ++i) {
        test_buffer.add(tmp_buffer, big_buffer_size);
        test_buffer.add(tmp_buffer, small_buffer_size);
        test_buffer.remove(tmp_buffer, mid_buffer_size);
        if (i % 2 == 0) {
            test_buffer.remove(tmp_buffer, big_buffer_size);
        }
    }

    fmt::print("The final size of the buffer: {} B.\n", test_buffer.size());
    fmt::print("The final list length of the buffer: {}.\n", test_buffer.chain_size());
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
