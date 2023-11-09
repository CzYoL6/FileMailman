file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h include/*.hpp)
add_library(core STATIC ${srcs})
target_link_libraries(core PUBLIC kcp)

target_include_directories(core PUBLIC include)
