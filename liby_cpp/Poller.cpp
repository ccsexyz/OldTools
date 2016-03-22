#include "Poller.h"
#include "Chanel.h"
#include <assert.h>
#include <sys/epoll.h>

Poller::Poller()
    : eventfd_(epoll_create1(EPOLL_CLOEXEC)),
      eventfd_ptr_(std::make_shared<FileHandle>(eventfd_)) {
    events_.resize(epollsize);
}

void Poller::loop_once(int waitMicroseconds) {
    assert(eventfd_ >= 0);
    int nfds = ::epoll_wait(eventfd_, getEventsPtr(), events_.size(),
                            waitMicroseconds);
    for (int i = 0; i < nfds; i++) {
        Chanel *ch = reinterpret_cast<Chanel *>(events_[i].data.ptr);
        if (ch == NULL)
            continue;
        ch->updateRevents(events_[i].events);
        ch->handleEvent();
    }
}
