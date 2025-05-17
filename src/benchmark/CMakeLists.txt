add_executable(intraprocess intraprocess.cpp)

add_executable(interprocess-producer ../benchmark/interprocess-producer.cpp)
target_link_libraries(interprocess-producer PRIVATE Boost::interprocess)

add_executable(interprocess-consumer ../benchmark/interprocess-consumer.cpp)
target_link_libraries(interprocess-consumer PRIVATE Boost::interprocess)
