//
// Created by JohnsonJohn on 15/9/24.
//
#ifndef ECHO_EPOLL_H
#define ECHO_EPOLL_H

#include <iostream>
#include <functional>
#include <new>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <errno.h>

typedef std::function<void(void*)> ev_callback;

class Event {
public:
    int fd;
    bool flag = true;
    void *ptr = nullptr;
    char *buf = nullptr;
    off_t offset = 0;
    off_t size = 4096;
    char *wbuf = nullptr;
    off_t woffset = 0;
    off_t wsize = 4096;
    struct epoll_event event;

    ev_callback write_callback = nullptr;
    ev_callback read_callback = nullptr;

    Event(int fd0);
    Event() = default;
    ~Event()
    {
        close(fd);
        if(buf != nullptr)
            delete[] buf;
        if(wbuf != nullptr)
            delete[] wbuf;
    }
    bool Read();
    bool Write();
};

typedef Event event_t;
typedef Event * pevent;
typedef std::function<void(struct epoll_event *, int )> epoll_event_handler;

class Epoll {
public:
    Epoll(int epoll_size0);
    ~Epoll()
    {
        delete[] events;
        close(epollfd);
    }

    int get_epoll_fd() const { return epollfd; }
    void epoll_main_event_loop(epoll_event_handler event_handler);
    void disable_loop() { flag = false; }
    void ctl(int op, int fd, struct epoll_event *e)
    {
        if(fd < 0 || op < 0 || e == nullptr) {
            std::cerr << "invalid args for epoll_ctl!" << std::endl;
            return;
        }

        if(epoll_ctl(epollfd, op, fd, e) < 0) {
            std::cerr << "epoll_ctl error!" << std::endl;
        }
    }
    void add(int fd, struct epoll_event *e)
    {
        ctl(EPOLL_CTL_ADD, fd, e);
    }
    void del(int fd, struct epoll_event *e)
    {
        ctl(EPOLL_CTL_DEL, fd, e);
    }
    void mod(int fd, struct epoll_event *e)
    {
        ctl(EPOLL_CTL_MOD, fd, e);
    }
private:
    bool flag = true;
    int epollfd = 0;
    int epoll_size;
    struct epoll_event *events = nullptr;
};


#endif //ECHO_EPOLL_H
