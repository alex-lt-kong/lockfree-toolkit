find_package(Boost REQUIRED COMPONENTS interprocess)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(producer producer.cpp)
target_link_libraries(producer PRIVATE Boost::interprocess)

add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE Boost::interprocess)
