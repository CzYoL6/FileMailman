//
// Created by zaiyi on 2023/11/7.
//

#include <gtest/gtest.h>
#include <core/SyncedBitmap.h>

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
TEST(CoreTest, SyncedMapTest){
    SyncedBitmap synced_bitmap(80, true);

    ASSERT_EQ(synced_bitmap.IsFull(), true);
    ASSERT_EQ(synced_bitmap.GetBit(1), true);

    synced_bitmap.SetBit(56, false);
    ASSERT_EQ(synced_bitmap.GetBit(56), false);
    ASSERT_EQ(synced_bitmap.IsFull(), false);


}