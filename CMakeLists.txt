cmake_minimum_required (VERSION 3.5)
project(unp_work)

add_subdirectory(src)
add_subdirectory(test)


add_library(thread_poll STATIC
        lib/thread_poll/thread_poll.cpp
        lib/thread_poll/thread_poll.h)

add_library(locker STATIC
        lib/locker/CLEvent.cpp
        lib/locker/CLEvent.h)

target_include_directories(unpWork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(unpWork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/thread_poll)
target_include_directories(unpWork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/locker)
