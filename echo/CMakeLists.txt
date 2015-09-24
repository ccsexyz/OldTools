cmake_minimum_required(VERSION 3.3)
project(echo)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp Epoll.cpp Epoll.h Implement.cpp Implement.h)
add_executable(echo ${SOURCE_FILES})