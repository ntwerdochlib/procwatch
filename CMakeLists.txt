cmake_minimum_required(VERSION 3.10)
project(procwatch)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_UNIT_TESTS "Build unit tests" ON)

if (BUILD_UNIT_TESTS)
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
  )

  FetchContent_MakeAvailable(googletest)
endif()

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
target_link_libraries(procwatch PRIVATE pw_socket)

set(srcs
  procwatchd.cpp
  ProcWatchSvr.h
  ProcWatchSvr.cpp
)

add_executable(process_watcher ${srcs})
target_link_libraries(process_watcher PRIVATE pw_socket pthread)

if (BUILD_UNIT_TESTS)
  set(srcs
    test/unit_tests.cpp
  )
  add_executable(unit_tests ${srcs})
 	target_compile_definitions(unit_tests PUBLIC
   UNIT_TESTING=1
	)
  target_include_directories(unit_tests PRIVATE
    ${gtest_SOURCE_DIR}/include
    ${gmock_SOURCE_DIR}/include
  )
  target_link_libraries(unit_tests
    gtest
    gmock
  )
endif()