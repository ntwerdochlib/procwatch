cmake_minimum_required(VERSION 3.10)
project(procwatch)

if (NOT LINUX)
  message(FATAL "Project builds for Linux only")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(docs)

set(srcs
  socket/Socket.h
  socket/UnixSocket.h
  socket/UnixSocket.cpp
  socket/NetlinkSocket.h
  socket/NetlinkSocket.cpp
)

add_library(pw_socket ${srcs})
target_include_directories(pw_socket PUBLIC ${CMAKE_CURRENT_LIST_DIR})

set(srcs
  procwatch.cpp
)

add_executable(procwatch ${srcs})
#target_include_directories(procwatch PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(procwatch PRIVATE pw_socket)

set(srcs
  procwatchd.cpp
  ProcWatchSvr.h
  ProcWatchSvr.cpp
)

add_executable(procwatchd ${srcs})
#target_include_directories(procwatchd PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(procwatchd PRIVATE pw_socket pthread)

add_executable(test_netlink test/TestNetlinkSocket.cpp)
target_link_libraries(test_netlink PRIVATE pw_socket)