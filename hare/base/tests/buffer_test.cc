#include <gtest/gtest.h>
#include <hare/base/io/buffer.h>

TEST(BufferTest, TestBufferAdd)
{
    hare::io::buffer::ptr shared_buffer(new hare::io::buffer());
    
    char* tmp = new char[HARE_LARGE_BUFFER - 1];
    EXPECT_EQ(shared_buffer->add(tmp, HARE_LARGE_BUFFER - 1), true);
    delete[] tmp;
    tmp = new char[HARE_LARGE_BUFFER - 1];
    EXPECT_EQ(shared_buffer->add(tmp, HARE_LARGE_BUFFER - 1), true);
    delete[] tmp;
}

auto main(int argc, char** argv) -> int
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}