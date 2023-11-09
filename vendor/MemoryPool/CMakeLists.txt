file(GLOB_RECURSE srcs CONFIGURE_DEPENDS *.cpp *.h)

add_library(memorypool STATIC ${srcs})
set_target_properties(memorypool PROPERTIES LINKER_LANGUAGE CXX)
