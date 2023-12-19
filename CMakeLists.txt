cmake_minimum_required(VERSION 3.25)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#set(BUILD_EXAMPLE ON)

# Set build type to DEBUG by default if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "DEBUG" CACHE STRING "Choose the type of build (DEBUG, RELEASE, DIST)" FORCE)
endif()

# Define macros for different build configurations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DWL_DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DWL_RELEASE)
elseif(CMAKE_BUILD_TYPE STREQUAL "Dist")
    add_definitions(-DWL_DIST)
endif()


project(FileMailman LANGUAGES C CXX)

add_subdirectory(vendor)
add_subdirectory(core)

enable_testing()
add_subdirectory(tests)
