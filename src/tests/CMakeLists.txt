add_executable(intraprocess-spsc-queue-test intraprocess-spsc-queue-test.cpp)
target_link_libraries(intraprocess-spsc-queue-test GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(intraprocess-spsc-queue-test)

add_executable(interprocess-spsc-queue-test interprocess-spsc-queue-test.cpp)
target_link_libraries(interprocess-spsc-queue-test GTest::gtest_main Boost::interprocess)
include(GoogleTest)
gtest_discover_tests(interprocess-spsc-queue-test)