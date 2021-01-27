set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}") # Allow to use FindXXX.cmake created by conan

# Next variable affect behaviour of Threads::Threads target
set(THREADS_PREFER_PTHREAD_FLAG TRUE) # prefer -pthread compiler and linker flag

set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(czmq REQUIRED)
find_package(zmqpp REQUIRED)

add_executable(beegfs_notify_test main.cpp)
target_link_libraries(beegfs_notify_test beegfs_notify_lib)
