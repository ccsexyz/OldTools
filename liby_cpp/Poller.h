#ifndef POLLERTEST_POLLER_H
#define POLLERTEST_POLLER_H

#include "BlockingQueue.h"
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
#include <vector>

namespace Liby {
class File;
class Chanel;
class EventQueue;
class TimerQueue;

using TimerId = uint64_t;

class Poller : clean_ {
public:
    Poller();
    virtual ~Poller();
    virtual void addChanel(Chanel *) = 0;
    virtual void updateChanel(Chanel *) = 0;
    virtual void removeChanel(Chanel *) = 0;
    virtual void loop_once() = 0;

#ifdef __APPLE__
    void handleTimeoutEvents();
    Timestamp getMinTimestamp();
#endif

    void init();
    void wakeup();
    void cancelAllTimer();
    void cancelTimer(TimerId id);
    TimerId runAt(const Timestamp &timestamp, const BasicHandler &handler);
    TimerId runAfter(const Timestamp &timestamp, const BasicHandler &handler);
    TimerId runEvery(const Timestamp &timestamp, const BasicHandler &handler);

    void runEventHandler(const BasicHandler &handler);

private:
    void runEveryHelper(TimerId id, const Timestamp &timestamp,
                        const BasicHandler &handler);

private:
    std::unique_ptr<EventQueue> eventQueue_;
    std::unique_ptr<TimerQueue> timerQueue_;
};

class PollerEpoll : public Poller {
public:
    static const int defaultEpollSize = 48;

    PollerEpoll();

    void loop_once() override;
    void addChanel(Chanel *chan) override;
    void updateChanel(Chanel *chan) override;
    void removeChanel(Chanel *chan) override;

#ifdef __linux__
    int pollerfd() {
        assert(pollerfd_ >= 0);
        return pollerfd_;
    }

private:
    struct epoll_event *getEventsPtr() {
        assert(events_.size() > 0);
        return &events_[0];
    }
    void translateEvents(struct epoll_event &event, Chanel *chan);

private:
    int pollerfd_ = -1;
    int eventsSize_ = 0;
    std::shared_ptr<File> pollerfp_;
    std::vector<struct epoll_event> events_;
#endif
};

class PollerSelect : public Poller {
public:
    PollerSelect();

    void loop_once() override;
    void addChanel(Chanel *chan) override;
    void updateChanel(Chanel *chan) override;
    void removeChanel(Chanel *chan) override;

private:
    fd_set rset_, wset_;
    int maxfd_ = -1;
    std::vector<Chanel *> chanels_;
};

class PollerPoll : public Poller {
public:
    static const int defaultPollSize = 48;

    PollerPoll();

    void loop_once() override;
    void addChanel(Chanel *chan) override;
    void updateChanel(Chanel *chan) override;
    void removeChanel(Chanel *chan) override;

private:
    struct pollfd *getPollfdPtr() {
        assert(pollfds_.size() > 0);
        return &pollfds_[0];
    }

private:
    int eventsSize_ = 0;
    std::vector<struct pollfd> pollfds_;
    std::unordered_map<int, Chanel *> chanels_;
};

class PollerKevent : public Poller {
public:
    PollerKevent();
    void addChanel(Chanel *) override;
    void updateChanel(Chanel *) override;
    void removeChanel(Chanel *) override;
    void loop_once() override;

#ifdef __APPLE__
private:
    void updateKevents();

private:
    int kq_ = -1;
    int eventsSize_ = 0;
    std::shared_ptr<File> keventPtr_;
    std::vector<struct kevent> events_;
    std::vector<struct kevent> changes_;
#endif
};
}

#endif // POLLERTEST_POLLER_H
