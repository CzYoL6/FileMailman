file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h include/*.hpp)
#add_library(core STATIC ${srcs})
add_library(core ${srcs})

target_link_libraries(core PUBLIC spdlog)
target_link_libraries(core PUBLIC ws2_32)

target_include_directories(core PUBLIC include)
target_include_directories(core PUBLIC ${CMAKE_SOURCE_DIR}/vendor/boost_1_83_0)



add_executable(core_test ${srcs})

target_link_libraries(core_test PUBLIC spdlog)
target_link_libraries(core_test PUBLIC ws2_32)

target_include_directories(core_test PUBLIC include)
target_include_directories(core_test PUBLIC ${CMAKE_SOURCE_DIR}/vendor/boost_1_83_0)
