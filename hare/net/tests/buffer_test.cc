#include <gtest/gtest.h>
#include <hare/net/buffer.h>

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>

TEST(BufferTest, testAddRemove)
{
    using hare::net::Buffer;
    buffer test_buffer {};

    constexpr std::size_t big_buffer_size { 0xc000 };
    constexpr std::size_t mid_buffer_size { 0x8000 };
    constexpr std::size_t small_buffer_size { 0x4000 };
    auto* tmp_buffer = new char[big_buffer_size];
    hare::detail::FillN(tmp_buffer, big_buffer_size, 'z');

    for (auto i = 0; i < 16; ++i) {
        test_buffer.Add(tmp_buffer, big_buffer_size);
        test_buffer.Add(tmp_buffer, small_buffer_size);
        test_buffer.Remove(tmp_buffer, mid_buffer_size);
        if (i % 2 == 0) {
            test_buffer.Remove(tmp_buffer, big_buffer_size);
        }
    }

    fmt::print("The final size of the buffer: {} B.\n", test_buffer.Size());
    fmt::print("The final list length of the buffer: {}.\n", test_buffer.ChainSize());
}

TEST(BufferTest, testAppend)
{
    using hare::net::Buffer;
    buffer test_buffer1 {};
    buffer test_buffer2 {};

    constexpr std::size_t big_buffer_size { 0xc000 };
    constexpr std::size_t mid_buffer_size { 0x8000 };
    constexpr std::size_t small_buffer_size { 0x4000 };
    auto* tmp_buffer = new char[big_buffer_size];
    hare::detail::FillN(tmp_buffer, big_buffer_size, 'z');

    for (auto i = 0; i < 16; ++i) {
        test_buffer1.Add(tmp_buffer, big_buffer_size);
        test_buffer1.Add(tmp_buffer, small_buffer_size);
        test_buffer1.Remove(tmp_buffer, mid_buffer_size);
        if (i % 2 == 0) {
            test_buffer1.Remove(tmp_buffer, big_buffer_size);
        }
    }

    test_buffer2.Add(tmp_buffer, big_buffer_size);
    test_buffer2.Append(test_buffer1);

    fmt::print("The final size of the buffer: {} B.\n", test_buffer1.Size());
    fmt::print("The final list length of the buffer: {}.\n", test_buffer1.ChainSize());
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
