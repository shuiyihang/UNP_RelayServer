cmake_minimum_required (VERSION 3.5)
project(unp_work)

# 设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")


# include_directories  路径增加到全局，对整个项目有效
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

add_subdirectory(src)
add_subdirectory(test)


add_library(thread_poll STATIC
        lib/thread_poll/thread_poll.cpp
        lib/thread_poll/thread_poll.h)

add_library(locker STATIC
        lib/locker/CLEvent.cpp
        lib/locker/CLEvent.h)

add_library(utils STATIC
        utils/utils.cpp
        utils/utils.h
        utils/session.cpp
        utils/session.h)




# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/thread_poll)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/locker)


# target_include_directories
# target_include_directories(unpWork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)
target_include_directories(unpWork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils)
target_include_directories(unpWork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/thread_poll)
target_include_directories(unpWork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib/locker)
