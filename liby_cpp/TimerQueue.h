#ifndef POLLERTEST_TIMERQUEUE_H
#define POLLERTEST_TIMERQUEUE_H

#include "BinaryHeap.h"
#include "util.h"
#include <atomic>
#include <set>

namespace Liby {
class Chanel;
class Poller;

using TimerId = uint64_t;

class Timer {
public:
    Timer() : timeout_(), handler_(), id_(0) {}
    Timer(const Timestamp &timeout, const BasicHandler &handler)
        : timeout_(timeout), handler_(handler), id_(timerIds_++) {}
    Timer(TimerId id, const Timestamp &timeout, const BasicHandler &handler)
        : id_(id), timeout_(timeout), handler_(handler) {}
    static TimerId getOneValidId() { return timerIds_++; }
    //        ~Timer() {
    //            info("Timer deconstruct!");
    //        }
    uint64_t id() const { return id_; }
    const Timestamp &timeout() const { return timeout_; }
    void runHandler() const { handler_(); }

private:
    static std::atomic<uint64_t> timerIds_;
    uint64_t id_;
    Timestamp timeout_;
    BasicHandler handler_;
};
inline bool operator<(const Timer &lhs, const Timer &rhs) {
    return lhs.timeout() < rhs.timeout() && lhs.id() < rhs.id();
}
class TimerQueue : clean_ {
public:
    TimerQueue(Poller *poller);

    void start();
    Poller *getPoller() const { return poller_; }

    void destroy();
    void cancel(TimerId id);
    void cancelAll();
    void insert(const Timer &timer);
    void setPoller(Poller *poller);
    void handleTimeoutEvents();

#ifdef __APPLE__
    Timestamp getMinTimestamp();
#endif

private:
    void updateTimerfd(const Timestamp &timeout);

private:
#ifdef __linux__
    int timerfd_;
    std::unique_ptr<Chanel> timerChan_;
#endif
    Poller *poller_;
    Liby::BinaryHeap<Timer> queue_;
};
}

#endif // POLLERTEST_TIMERQUEUE_H
