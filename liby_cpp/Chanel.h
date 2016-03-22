#ifndef LIBY_CHANEL_H
#define LIBY_CHANEL_H

#include "FileHandle.h"
#include "Poller.h"
#include "util.h"

class Chanel {
public:
    using ChanelHandler = std::function<void()>;

    Chanel() = default;
    Chanel(Poller *poller, FileHandlePtr fh, int events)
        : poller_(poller), fh_(fh), events_(events) {}
    ~Chanel() { removeChanelFromPoller(); }

    void setPoller(Poller *poller) { poller_ = poller; }
    void setFileHandlePtr(FileHandlePtr fh) { fh_ = fh; }
    void setEvents(int events) { events_ = events; }

    FileHandlePtr &fileHandlePtr() { return fh_; }

    void runHandler() { handler_(); }

    void updateRevents(int revents) { revents_ = revents; }

    bool isError() const {
        return revents_ & EPOLLHUP || revents_ & EPOLLRDHUP ||
               revents_ & EPOLLERR;
    }
    bool Readable() const { return revents_ & EPOLLIN; }
    bool Writable() const { return revents_ & EPOLLOUT; }

    void enableRead(bool flag = true) {
        events_ = flag ? (events_ | EPOLLIN) : (events_ & ~EPOLLIN);
    }
    void enableWrit(bool flag = true) {
        events_ = flag ? (events_ | EPOLLOUT) : (events_ & ~EPOLLOUT);
    }
    void flushChanelState() { ctl(EPOLL_CTL_MOD); }

    void handleEvent();
    void setReadEventCallback(const ChanelHandler &cb) {
        readEventCallback_ = cb;
    }
    void setWritEventCallback(const ChanelHandler &cb) {
        writEventCallback_ = cb;
    }
    void setErroEventCallback(const ChanelHandler &cb) {
        erroEventCallback_ = cb;
    }

    void removeChanelFromPoller() {
        ctl(EPOLL_CTL_DEL);
        poller_ = NULL;
    }

    void modChanelOfPoller() { ctl(EPOLL_CTL_MOD); }

    void addChanelToPoller() { ctl(EPOLL_CTL_ADD); }

    Poller *getPoller() const { return poller_; }

private:
    void ctl(int op);

private:
    ChanelHandler readEventCallback_;
    ChanelHandler writEventCallback_;
    ChanelHandler erroEventCallback_;

    int events_;
    int revents_;
    Poller *poller_;
    FileHandlePtr fh_;
    ChanelHandler handler_;
};

using ChanelPtr = std::shared_ptr<Chanel>;

#endif // LIBY_CHANEL_H
