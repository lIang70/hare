#include <hare/net/buffer.h>

#include <gtest/gtest.h>

TEST(buffer_test, testBufferAdd)
{
    std::shared_ptr<hare::net::Buffer> shared_buffer(new hare::net::Buffer());
    
    char* tmp = new char[1024];
    EXPECT_EQ(shared_buffer->add(tmp, 1024), true);
    delete[] tmp;
    tmp = new char[4096];
    EXPECT_EQ(shared_buffer->add(tmp, 4096), true);
    delete[] tmp;
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}