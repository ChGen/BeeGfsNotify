cmake_minimum_required(VERSION 3.16)
project(beegfs_notify)

set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}") # Allow to use FindXXX.cmake created by conan

# Next variable affect behaviour of Threads::Threads target
set(THREADS_PREFER_PTHREAD_FLAG TRUE) # prefer -pthread compiler and linker flag

find_package(czmq REQUIRED)
find_package(hiredis REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(cryptopp REQUIRED)
find_package(zmqpp REQUIRED)

add_subdirectory(beegfs_notify_server_lib)
add_subdirectory(beegfs_notify_client_lib)
add_subdirectory(beegfs_notify_test)

