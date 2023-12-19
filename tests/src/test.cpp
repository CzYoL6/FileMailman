//
// Created by zaiyi on 2023/11/7.
//

#include <gtest/gtest.h>
#include <core/BufferBlock.h>
#include <core/Message.h>
#include <core/Event.hpp>

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
    BufferBlock buffer_block(1024, 10240, temp);

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
    ASSERT_EQ(std::get<0>(Message::GetReceiverHeader(std::span<char>(msg, msg + sizeof(msg)))) == Message::ReceiverMsgHeader::kBeginTransfer, true);
    ASSERT_EQ(std::get<0>(Message::GetSenderHeader(std::span<char>(msg, msg + sizeof(msg)))) == Message::SenderMsgHeader::kBeginTransferAck_FileMeta, true);

}

TEST(CoreTest, VectorCastTest){
    std::vector<unsigned char> m = {};
    auto& m_ = reinterpret_cast<std::vector<int>&>(m);
    m_.push_back(2);

    ASSERT_EQ(m.size(), 4);
    ASSERT_EQ(m[0], (uint8_t)0x02);
    ASSERT_EQ(m[1], (uint8_t)0x00);
    ASSERT_EQ(m[2], (uint8_t)0x00);
    ASSERT_EQ(m[3], (uint8_t)0x00);

    m.clear();
    m.push_back(0x01);
    std::vector<int> n = {2};
    auto& n_ = reinterpret_cast<std::vector<unsigned char>&>(n);
    m.insert(m.end(), n_.begin(), n_.end());
    ASSERT_EQ(m.size(), 5);
    ASSERT_EQ(m[0], (uint8_t)0x01);
    ASSERT_EQ(m[1], (uint8_t)0x02);
    ASSERT_EQ(m[2], (uint8_t)0x00);
    ASSERT_EQ(m[3], (uint8_t)0x00);
    ASSERT_EQ(m[4], (uint8_t)0x00);

}

TEST(CoreTest, BufferBlockReadWriteTest){
    auto t = boost::asio::io_context{};
    BufferBlock bufferBlock(1, 8, t);

}

TEST(CoreTest, EventTestNoArgument){
    Event add_event;
    int a = 1;
    int b = 2;
    std::function<void()> e1,e2;
    e1=[&](){
        a++;
    };
    e2=[&](){
        b++;
    };
    add_event.Subscribe(e1);
    add_event.Subscribe(e2);

    add_event.Trigger();
    ASSERT_EQ(a, 2);
    ASSERT_EQ(b, 3);

}
TEST(CoreTest, EventTestWithArgument){
    Event<int,int, int&> add_event;
    int a = 1;
    int b = 2;
    int c;
    std::function<void(int,int, int&)> e1;
    e1=[&](int m, int n,int &r){
        r = m + n;
    };
    add_event.Subscribe(e1);

    add_event.Trigger(a, b, c);
    ASSERT_EQ(c, 3);

}
