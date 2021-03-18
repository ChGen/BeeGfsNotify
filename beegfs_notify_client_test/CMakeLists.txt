set(CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}") # Allow to use FindXXX.cmake created by conan

# Next variable affect behaviour of Threads::Threads target
set(THREADS_PREFER_PTHREAD_FLAG TRUE) # prefer -pthread compiler and linker flag

find_package(Threads REQUIRED)
find_package(ZeroMQ REQUIRED)
find_package(czmq REQUIRED)
find_package(zmqpp REQUIRED)

add_executable(beegfs_client_test
    client_test.cpp
)

#target_compile_options(beegfs_client_test PRIVATE "-fsanitize=address")
#target_link_options(beegfs_client_test PRIVATE "-fsanitize=address")

target_include_directories(beegfs_client_test PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/../Common"
    "${CMAKE_CURRENT_SOURCE_DIR}/../beegfs_notify_client_lib"
)

target_link_libraries(beegfs_client_test PRIVATE
                beegfs_notify_client_lib
		Threads::Threads
		zmq
		zmqpp::zmqpp
)
