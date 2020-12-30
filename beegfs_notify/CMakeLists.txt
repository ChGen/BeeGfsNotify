cmake_minimum_required(VERSION 3.10)

project(beegfs_notify)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)
find_package(ZeroMQ 4.3.3 REQUIRED)
find_package(czmq 4.2.0 REQUIRED)
find_package(zmqpp 4.2.0 REQUIRED)

add_executable(beegfs_notify
    main.cpp
    BeegfsFileEventLog.cpp
    DfsNotifier.cpp
    )

target_link_libraries(beegfs_notify
    Threads::Threads
    zmq
    zmqpp::zmqpp
    czmq::czmq-static
    )

add_library(beegfs_notify_client STATIC DfsEventsWatcher.cpp)
target_link_libraries(beegfs_notify_client
    Threads::Threads
    zmq
    zmqpp::zmqpp
    czmq::czmq-static
    )
