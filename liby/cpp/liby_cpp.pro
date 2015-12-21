TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    socket.cpp \
    log.cpp \
    base.cpp \
    eventloop.cpp \
    connection.cpp \
    server.cpp

HEADERS += \
    socket.h \
    log.h \
    base.h \
    liby.h \
    eventloop.h \
    connection.h \
    server.h \
    buffer.h \
    io_task.h
