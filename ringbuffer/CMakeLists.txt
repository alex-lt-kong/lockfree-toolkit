cmake_minimum_required(VERSION 3.30)
project(ringbuffer)

set(CMAKE_CXX_STANDARD 23)

find_package(GTest CONFIG REQUIRED COMPONENTS GTest GMock)
include_directories(${GTEST_INCLUDE_DIRS})

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

add_executable(rightbuffer-spsc-impl-test rightbuffer-spsc-impl-test.cpp)
target_link_libraries(rightbuffer-spsc-impl-test GTest::gtest GTest::gtest_main GTest::gmock)