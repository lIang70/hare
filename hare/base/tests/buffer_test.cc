#include <gtest/gtest.h>
#include <hare/base/io/buffer.h>

TEST(BufferTest, TestBufferAppend)
{
    hare::io::buffer buffer_a;
    hare::io::buffer buffer_b;
    
    char* tmp = new char[HARE_LARGE_BUFFER - 1];
    EXPECT_EQ(buffer_b.add(tmp, HARE_LARGE_BUFFER - 1), true);
    delete[] tmp;
    tmp = new char[HARE_LARGE_BUFFER - 1];
    EXPECT_EQ(buffer_a.add(tmp, HARE_LARGE_BUFFER - 1), true);
    delete[] tmp;

    buffer_a.append(buffer_b);
}

TEST(BufferTest, TestBufferAdd)
{
    hare::io::buffer buffer;
    
    char* tmp = new char[HARE_LARGE_BUFFER - 1];
    EXPECT_EQ(buffer.add(tmp, HARE_LARGE_BUFFER - 1), true);
    delete[] tmp;
    tmp = new char[HARE_LARGE_BUFFER - 1];
    EXPECT_EQ(buffer.add(tmp, HARE_LARGE_BUFFER - 1), true);
    delete[] tmp;
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}