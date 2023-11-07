file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h include/*.hpp)
add_library(core STATIC ${srcs})
#target_link_libraries(GGgui PUBLIC glfw)

target_include_directories(core PUBLIC include)
