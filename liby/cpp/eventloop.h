#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "base.h"
#include <sys/epoll.h>

#define DEFAULT_EPOLLSIZE (10240)

class Event;
class EventLoop;

using event_hander = std::function<void(std::vector<std::shared_ptr<Event>> &)>;
using EVENT_PTR = std::shared_ptr<Event>;
using EVENTLOOP_PTR = std::shared_ptr<EventLoop>;

class Event : public std::enable_shared_from_this<Event>
{
public:
    explicit Event(std::function<void(std::shared_ptr<Event>)> func0) : func(func0)
    {
        event_ptr = &event;
        event_ptr->data.ptr = reinterpret_cast<void *>(this);
    }

    struct epoll_event *get_event_ptr() { return event_ptr; }
    void operator()() { if(func) func(shared_from_this()); }

private:
    struct epoll_event event;
    struct epoll_event *event_ptr;
    std::function<void(std::shared_ptr<Event>)> func;
};

class EventLoop : public std::enable_shared_from_this<EventLoop>
{
public:
    explicit EventLoop(int epollsize0 = DEFAULT_EPOLLSIZE);
    void main_loop(event_hander handler = nullptr);
    void default_handler(std::vector<std::shared_ptr<Event>> &e);
    void add(FD_HOLDER fd0, EVENT_PTR ep);
    void mod(FD_HOLDER fd0, EVENT_PTR ep);
    void ctl(int op, int fd0, struct epoll_event *e);
    void disable_loop() { flag = false; }
private:
    int epollsize;
    bool flag = false;
    int ec = 0; //error_code
    //int running_servers = 0;

    int epfd;
    FD_HOLDER epfd_;

    std::vector<struct epoll_event> events;
};

#endif // EVENTLOOP_H
