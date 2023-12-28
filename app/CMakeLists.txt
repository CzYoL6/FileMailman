file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h include/*.hpp)
add_executable(filemailman ${srcs} )
target_link_libraries(filemailman PUBLIC core)
target_include_directories(filemailman PUBLIC include)