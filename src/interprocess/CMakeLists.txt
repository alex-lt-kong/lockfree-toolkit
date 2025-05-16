

add_executable(producer producer.cpp)
target_link_libraries(producer PRIVATE Boost::interprocess)

add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE Boost::interprocess)
