#include "TimerQueue.h"
#include "Chanel.h"
#include "File.h"
#include "Logger.h"
#include "Poller.h"
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef __linux__
#include <sys/timerfd.h>
#endif

using namespace Liby;

std::atomic<uint64_t> Timer::timerIds_(1); // tiemrId will never be zero, so zero timerId is invalid

TimerQueue::TimerQueue(Poller *poller) : poller_(poller) {
#ifdef __linux__
    fatalif(poller_ == nullptr, "Poller cannot be nullptr");
    timerfd_ = ::timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC | TFD_NONBLOCK);
    fatalif(timerfd_ < 0, "timerfd_create: %s", ::strerror(errno));
    auto timerfp_ = std::make_shared<File>(timerfd_);
    timerChan_ = std::make_unique<Chanel>(poller_, timerfp_);
#endif
}

TimerQueue::~TimerQueue() {
#ifdef __linux__
    //　使timerChan_析构时不去调用Poller虚基类的虚函数
    if (timerChan_) {
        timerChan_->setPoller(nullptr);
    }
#endif
}

void TimerQueue::updateTimerfd(const Timestamp &timeout) {
#ifdef __linux
    struct itimerspec new_timer;
    new_timer.it_value.tv_sec = timeout.sec();
    new_timer.it_value.tv_nsec = timeout.usec() * 1000;
    new_timer.it_interval = {0, 0};
    int ret = ::timerfd_settime(timerfd_, TFD_TIMER_ABSTIME, &new_timer, NULL);
    errorif(ret < 0, "timer_settime: %s", ::strerror(errno));
#endif
}

void TimerQueue::start() {
#ifdef __linux__
    timerChan_->enableRead();
    timerChan_->setErroEventCallback([this] { destroy(); });
    timerChan_->setReadEventCallback([this] {
        if (timerfd_ < 0) {
            error("timerfd < 0");
        }

        uint64_t n;
        while (1) {
            int ret = ::read(timerfd_, &n, sizeof(n));
            if (ret <= 0)
                break;
        }

        handleTimeoutEvents();
    });
    timerChan_->addChanel();
#endif
}

void TimerQueue::destroy() {
#ifdef __linux__
    timerfd_ = -1;
    timerChan_.reset();
#endif
    queue_.clear();
}

void TimerQueue::cancel(TimerId id) {
    if(id == 0)
        return;
    queue_.erase_if([id](const Timer &timer) { return timer.id() == id; });
}

void TimerQueue::cancelAll() { queue_.clear(); }

void TimerQueue::handleTimeoutEvents() {
    assert(poller_);
    if (queue_.empty()) {
        return;
    }

    auto now = Timestamp::now();
    while (!queue_.empty()) {
        const Timer &minTimer = queue_.find_min();
        if (now < minTimer.timeout()) {
            break;
        }
//        minTimer.runHandler();
        auto savedMinTimer = minTimer;
        // warning : minTimer handler may delete itself, so make a copy
        debug("delete timer id = %lu\n", minTimer.id());
        queue_.delete_min();
        savedMinTimer.runHandler();
    }
    if (!queue_.empty()) {
        updateTimerfd(queue_.find_min().timeout());
    }
}

void TimerQueue::insert(const Timer &timer) {
    debug("add timer id = %lu", timer.id());
    if(timer.id() == 0) // ignore all timer which id is zero
        return;
    if (queue_.empty()) {
        updateTimerfd(timer.timeout());
    }
    queue_.insert(timer);
}

void TimerQueue::setPoller(Poller *poller) {
    poller_ = poller;
#ifdef __linux__
    timerChan_->setPoller(poller_);
#endif
}

#ifdef __APPLE__
Timestamp TimerQueue::getMinTimestamp() {
    if (queue_.empty()) {
        return Timestamp::invalidTime();
    } else {
        const Timer &minTimer = queue_.find_min();
        return minTimer.timeout();
    }
}
#endif
