add_executable(intraprocess-spsc-queue-test spsc-queue-test.cpp)
target_link_libraries(intraprocess-spsc-queue-test GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(intraprocess-spsc-queue-test)