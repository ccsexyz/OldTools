#include "Chanel.h"

void Chanel::handleEvent() {
    if (isError() && erroEventCallback_) {
        erroEventCallback_();
    } else if (Readable() && readEventCallback_) {
        readEventCallback_();
    } else if (Writable() && writEventCallback_) {
        writEventCallback_();
    }
}

void Chanel::ctl(int op) {
    int fd = fh_->fd();
    if (poller_ == NULL || fd < 0)
        return;
    struct epoll_event ev;
    ev.data.ptr = reinterpret_cast<void *>(this);
    ev.events = events_;
    ::epoll_ctl(poller_->eventfd(), op, fd, &ev);
}
