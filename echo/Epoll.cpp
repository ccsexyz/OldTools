//
// Created by JohnsonJohn on 15/9/24.
//

#include "Epoll.h"

Epoll::Epoll(int epoll_size0)
{
    if(epoll_size0 > 0) {
        epoll_size = epoll_size0;
    } else {
        epoll_size = 10240;
    }

    epollfd = epoll_create(epoll_size);
    if(epollfd < 0) {
        std::cerr << "epoll_create error" << std::endl;
        std::exit(1);
    }

    events = new struct epoll_event[epoll_size];
    if(events == nullptr) {
        std::cerr << "bad alloc for events" << std::endl;
        std::exit(1);
    }
}

void
Epoll::epoll_main_event_loop(epoll_event_handler event_handler)
{
    if(epollfd <= 0)
        return;
    for(int nfds; flag; ) {
        nfds = epoll_wait(epollfd, events, epoll_size, -1);
        if(nfds <= 0) continue;
        event_handler(events, nfds);
    }
}

Event::Event(int fd0)
{
    if(fd0 <= 0) return;
    fd = fd0;
    try {
        buf = new char[size];
        wbuf = new char[wsize];
    } catch (...) {
        std::cerr << "bad alloc for Event" << std::endl;
        std::exit(1);
    }

    event.data.ptr = static_cast<void *>(this);
    event.events = EPOLLIN | EPOLLET;
}

bool
Event::Read()
{
    int ret = read(fd, buf, size);
    if(ret > 0) {
        offset = ret;
        if(read_callback != nullptr)
            read_callback(NULL);
        return true;
    } else {
        if(ret == 0 || errno != EAGAIN) {
            flag = false;
        }
        read_callback(NULL);
        return false;
    }
}

bool
Event::Write()
{
    int ret = write(fd, wbuf, offset - woffset);
    if(ret == offset - woffset) {
        offset = 0;
        if(write_callback != nullptr)
            write_callback(NULL);
        return true;
    } else {
        if(errno != EAGAIN) {
            flag = false;
        } else if(ret > 0) {
            offset += ret;
        }
        if(write_callback != nullptr)
            write_callback(NULL);
        return false;
    }
}
