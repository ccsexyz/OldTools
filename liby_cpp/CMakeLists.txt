cmake_minimum_required(VERSION 3.2)
project(PollerTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g")

set(SOURCE_FILES Logger.cpp Logger.h BlockingQueue.h util.cpp util.h File.cpp File.h Poller.cpp Poller.h Chanel.cpp Chanel.h Buffer.cpp Buffer.h Connection.cpp Connection.h TcpServer.cpp TcpServer.h TcpClient.cpp TcpClient.h EventLoop.cpp EventLoop.h EventLoop.cpp Liby.h TimerQueue.cpp TimerQueue.h BinaryHeap.h EventQueue.cpp EventQueue.h http/Request.cpp http/Request.h http/Reply.cpp http/Reply.h tests/tcp_proxy.cpp http/HttpServer.h http/HttpServer.cpp)
add_executable(PollerTest ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(PollerTest pthread)
