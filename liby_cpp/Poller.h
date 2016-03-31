#ifndef POLLERTEST_POLLER_H
#define POLLERTEST_POLLER_H

#include "BlockingQueue.h"
#include "Chanel.h"
#include "util.h"
#include <atomic>
#ifdef __linux__
#include <sys/epoll.h>
#elif defined(__APPLE__)
#include <sys/event.h>
#endif
#include <sys/poll.h>
#include <sys/select.h>
#include <unordered_map>

namespace Liby {
class Poller : clean_ {
public:
    Poller(){};
    virtual ~Poller(){};
    virtual void addChanel(Chanel *) = 0;
    virtual void updateChanel(Chanel *) = 0;
    virtual void removeChanel(Chanel *) = 0;
    virtual void loop_once(int interMs = -1) = 0;
    virtual void runEventHandler(BasicHandler handler) = 0;
    virtual void wakeup() = 0;
};

class PollerEpoll : public Poller {
public:
    static const int defaultEpollSize = 48;

    PollerEpoll();

    void loop_once(int interMs = -1) override; // microseconds
    void addChanel(Chanel *chan) override;
    void updateChanel(Chanel *chan) override;
    void removeChanel(Chanel *chan) override;
    void runEventHandler(BasicHandler handler) override;
    void wakeup() override;

#ifdef __linux__
    int pollerfd() {
        assert(pollerfd_ >= 0);
        return pollerfd_;
    }

private:
    void initEventfd();
    struct epoll_event *getEventsPtr() {
        assert(events_.size() > 0);
        return &events_[0];
    }
    void translateEvents(struct epoll_event &event, Chanel *chan);

private:
    int pollerfd_ = -1;
    int eventfd_ = -1;
    int eventsSize_ = 0;
    FilePtr pollerfp_;
    FilePtr eventfp_;
    ChanelPtr eventChanelPtr_;
    std::vector<struct epoll_event> events_;
    std::shared_ptr<BlockingQueue<BasicHandler>> eventHandlersPtr_;
#endif
};

class PollerSelect : public Poller {
public:
    PollerSelect();

    void loop_once(int interMs = -1) override; // microseconds
    void addChanel(Chanel *chan) override;
    void updateChanel(Chanel *chan) override;
    void removeChanel(Chanel *chan) override;
    void runEventHandler(BasicHandler handler) override;
    void wakeup() override;

private:
    void initEventfd();

private:
    fd_set rset_, wset_;
    int eventfd_ = -1;
    int maxfd_ = -1;
    FilePtr eventfp_;
#ifdef __APPLE__
    int eventfd2_ = -1;
    FilePtr eventfp2_;
#endif
    ChanelPtr eventChanelPtr_;
    std::vector<Chanel *> chanels_;
    std::shared_ptr<BlockingQueue<BasicHandler>> eventHandlersPtr_;
};

class PollerPoll : public Poller {
public:
    static const int defaultPollSize = 48;

    PollerPoll();

    void loop_once(int interMs = -1) override; // microseconds
    void addChanel(Chanel *chan) override;
    void updateChanel(Chanel *chan) override;
    void removeChanel(Chanel *chan) override;
    void runEventHandler(BasicHandler handler) override;
    void wakeup() override;

private:
    void initEventfd();
    struct pollfd *getPollfdPtr() {
        assert(pollfds_.size() > 0);
        return &pollfds_[0];
    }

private:
    int eventfd_ = -1;
#ifdef __APPLE__
    int eventfd2_ = -1;
    FilePtr eventfp2_;
#endif
    int eventsSize_ = 0;
    FilePtr eventfp_;
    ChanelPtr eventChanelPtr_;
    std::vector<struct pollfd> pollfds_;
    std::unordered_map<int, Chanel *> chanels_;
    std::shared_ptr<BlockingQueue<BasicHandler>> eventHandlersPtr_;
};

class PollerKevent : public Poller {
public:
    PollerKevent();
    void addChanel(Chanel *) override;
    void updateChanel(Chanel *) override;
    void removeChanel(Chanel *) override;
    void loop_once(int interMs = -1) override;
    void runEventHandler(BasicHandler handler) override;
    void wakeup() override;

#ifdef __APPLE__
private:
    void initEventfd();
    void updateKevents();

private:
    int kq_ = -1;
    int eventfd_ = -1;
    int eventfd2_ = -1;
    int eventsSize_ = 0;
    FilePtr eventfp_;
    FilePtr eventfp2_;
    FilePtr keventPtr_;
    ChanelPtr eventChanelPtr_;
    std::vector<struct kevent> events_;
    std::vector<struct kevent> changes_;
    std::shared_ptr<BlockingQueue<BasicHandler>> eventHandlersPtr_;
#endif
};
}

#endif // POLLERTEST_POLLER_H
