add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE Boost::interprocess)
add_executable(producer producer.cpp)
target_link_libraries(producer PRIVATE Boost::interprocess)

add_executable(interprocess-producer ./interprocess-producer.cpp)
target_link_libraries(interprocess-producer PRIVATE Boost::interprocess)

add_executable(interprocess-consumer ./interprocess-consumer.cpp)
target_link_libraries(interprocess-consumer PRIVATE Boost::interprocess)

add_executable(intraprocess ./intraprocess.cpp)
#target_link_libraries(intraprocess PRIVATE Boost::interprocess)
