#include "eventloop.h"
#include "log.h"
#include "time.h"

EventLoop::EventLoop(int epollsize0) : epollsize(epollsize0)
{
    events.resize(epollsize);
    epfd = epoll_create(epollsize);
    epfd_ = std::make_shared<fd_holder>(epfd);
}

void EventLoop::main_loop(event_hander handler)
{
    flag = true;
    struct epoll_event *p = &events[0];

    while(flag) {
        int nfds = epoll_wait(epfd, p, epollsize, -1);
        if(nfds <= 0) continue;

        std::vector<std::shared_ptr<Event>> e;
        e.reserve(nfds);
        for(int i = 0; i < nfds; i++) {
            Event *t = reinterpret_cast<Event *>(p[i].data.ptr);
            *(t->get_event_ptr()) = p[i];
            e.push_back(t->shared_from_this());
        }

        if(handler)
            handler(e);
        else
            default_handler(e);
    }
}

void EventLoop::default_handler(std::vector<std::shared_ptr<Event> > &e)
{
    for(auto &x : e) { (*x)(); }
}

void EventLoop::ctl(int op, int fd0, epoll_event *e)
{
    if(epoll_ctl(epfd, op, fd0, e) < 0) {
        log_err("epoll_ctl error");
    }
}

void EventLoop::add(FD_HOLDER fd0, EVENT_PTR ep)
{
    ctl(EPOLL_CTL_ADD, fd0->operator ()(), ep->get_event_ptr());
}

void EventLoop::mod(FD_HOLDER fd0, EVENT_PTR ep)
{
    ctl(EPOLL_CTL_MOD, fd0->operator ()(), ep->get_event_ptr());
}
