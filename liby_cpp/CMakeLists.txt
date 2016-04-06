cmake_minimum_required(VERSION 3.3)
project(PollerTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES Logger.cpp Logger.h BlockingQueue.h util.cpp util.h File.cpp File.h Poller.cpp Poller.h Chanel.cpp Chanel.h Buffer.cpp Buffer.h Connection.cpp Connection.h TcpServer.cpp TcpServer.h TcpClient.cpp TcpClient.h EventLoop.cpp EventLoop.h EventLoop.cpp Liby.h tests/tcp_proxy.cpp TimerQueue.cpp TimerQueue.h BinaryHeap.h EventQueue.cpp EventQueue.h)
add_executable(PollerTest ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(PollerTest pthread)
