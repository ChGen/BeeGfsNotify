cmake_minimum_required(VERSION 3.10)

project(beegfs_notify)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(beegfs_notify main.cpp
    BeegfsFileEventLog.cpp
    )
#target_compile_options(beegfs_notify PUBLIC "-stdlib=libc++")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
target_link_libraries(beegfs_notify "-lzmq -lczmq -lzmqpp -lpthread -lstdc++fs")

add_executable(client_test client_test.cpp)
#target_compile_options(client_test PUBLIC "-stdlib=libc++")
target_link_libraries(client_test "-lzmq -lczmq -lzmqpp")
