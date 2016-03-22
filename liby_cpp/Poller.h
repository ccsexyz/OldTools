#ifndef LIBY_POLLER_H
#define LIBY_POLLER_H

#include "FileHandle.h"
#include <sys/epoll.h>
#include <vector>

class Poller final {
public:
    static const int epollsize = 10240;

    Poller();
    ~Poller() = default;

    void loop_once(int waitMicroseconds = -1);

    struct epoll_event *getEventsPtr() {
        return &events_[0];
    }

    int eventfd() const { return eventfd_; }

private:
    int eventfd_;
    FileHandlePtr eventfd_ptr_;
    std::vector<struct epoll_event> events_;
    int eventsSize_ = 0;
};

#endif // LIBY_POLLER_H
