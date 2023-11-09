file(GLOB_RECURSE srcs CONFIGURE_DEPENDS *.cpp *.h)

add_library(kcp STATIC ${srcs})
set_target_properties(kcp PROPERTIES LINKER_LANGUAGE CXX)
