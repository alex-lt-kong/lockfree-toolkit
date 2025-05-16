cmake_minimum_required(VERSION 3.12)
project(lockfree-toolkit)

set(CMAKE_CXX_STANDARD 23)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#include(cmake/compiler-flags.cmake)
#include(cmake/sanitizers.cmake)
#include(cmake/git.cmake)


#set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/interprocess/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/intraprocess/)
