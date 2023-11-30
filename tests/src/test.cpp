//
// Created by zaiyi on 2023/11/7.
//

#include <gtest/gtest.h>
#include <core/BufferBlock.h>
#include <core/Message.h>

//class TestCore : public ::testing::Test{
//public:
//    Camera a;
//public:
//    virtual void SetUp() override{
//        a = Camera(45.0f, 0.1f, 100.0f);
//    };
//
////    virtual void TearDown() override{
////
////    }
//};

//TEST_F(TestCamera, TEST_PROJECTION){
//
//    ASSERT_EQ(a + b, ans) ;
//}
//
TEST(CoreTest, BufferBlockTest){
    boost::asio::io_context temp;
    BufferBlock buffer_block(1024, 10248, temp);

    ASSERT_EQ(buffer_block.IsSliceDone(1), false);
    buffer_block.SetSliceDone(1);
    ASSERT_EQ(buffer_block.IsSliceDone(1), true);

    for(int i = 0; i < 11; i++){
        buffer_block.SetSliceDone(i);
    }
    ASSERT_EQ(buffer_block.SliceAllDone(), true);

}
TEST(CoreTest, MessageHeaderGetterTest){
    char msg[] = {0x01, 0x12, 0x13};
    ASSERT_EQ(std::get<0>(Message::GetClientHeader(std::span<char>(msg, msg+sizeof(msg)))) == Message::ClientMsgHeader::kBeginTransfer, true);
    ASSERT_EQ(std::get<0>(Message::GetServerHeader(std::span<char>(msg, msg+sizeof(msg)))) == Message::ServerMsgHeader::kBeginTransferAck_FileMeta, true);

}