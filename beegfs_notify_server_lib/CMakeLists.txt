set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}") # Allow to use FindXXX.cmake created by conan

# Next variable affect behaviour of Threads::Threads target
set(THREADS_PREFER_PTHREAD_FLAG TRUE) # prefer -pthread compiler and linker flag

find_package(Threads REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(czmq REQUIRED)
find_package(zmqpp REQUIRED)

add_library(beegfs_notify_lib
    BeegfsFileEventLog.cpp
    DfsNotifier.cpp
)

target_include_directories(beegfs_notify_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../Common"
)

target_link_libraries(beegfs_notify_lib PUBLIC
		Threads::Threads
		zmq
		zmqpp::zmqpp
		czmq::czmq-static
)
