file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp include/*.h src/*.hpp include/*.hpp)

add_library(asio STATIC ${srcs})
set_target_properties(asio PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(asio PUBLIC include)
