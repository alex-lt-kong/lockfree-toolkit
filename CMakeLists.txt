cmake_minimum_required(VERSION 3.12)
project(lockfree-toolkit)

set(CMAKE_CXX_STANDARD 23)


find_package(GTest CONFIG REQUIRED COMPONENTS GTest GMock)
include_directories(${GTEST_INCLUDE_DIRS})
find_package(Boost REQUIRED COMPONENTS interprocess)
include_directories(${Boost_INCLUDE_DIRS})
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/interprocess/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/intraprocess/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark/)
