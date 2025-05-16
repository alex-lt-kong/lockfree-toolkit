find_package(GTest CONFIG REQUIRED COMPONENTS GTest GMock)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(intraprocess-spsc-queue-test spsc-queue-test.cpp)
target_link_libraries(intraprocess-spsc-queue-test GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(intraprocess-spsc-queue-test)