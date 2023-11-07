file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h)
add_executable(mtest ${srcs})
target_link_libraries(mtest
        PRIVATE
        GTest::gtest_main
        core
        )

add_test(gtests mtest)