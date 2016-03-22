cmake_minimum_required(VERSION 3.2.2)
project(Liby)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(SOURCE_FILES main.cpp util.cpp util.h test/Timestamp_test.h test/test.cpp test/test.h test/DeferCaller_test.h test/Sinal_test.h FileHandle.cpp FileHandle.h test/FileHandle_test.h Connection.cpp Connection.h Chanel.cpp Chanel.h Poller.cpp Poller.h BlockingQueue.h TcpServer.cpp TcpServer.h TcpClient.cpp TcpClient.h EventLoop.cpp EventLoop.h)
add_executable(Liby ${SOURCE_FILES})
TARGET_LINK_LIBRARIES(Liby pthread)
