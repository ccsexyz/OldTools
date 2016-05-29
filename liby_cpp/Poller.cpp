#include "Poller.h"
#include "Channel.h"
#include "EventQueue.h"
#include "TimerQueue.h"
#include <string.h>
#ifdef __linux__
#include <sys/epoll.h>
#include <sys/eventfd.h>
#elif defined(__APPLE__)
#include <sys/event.h>
#include <sys/time.h>
#include <sys/types.h>
#endif

using namespace Liby;

Poller::Poller() {
    eventQueue_ = std::make_unique<EventQueue>(this);
    timerQueue_ = std::make_unique<TimerQueue>(this);

    channels_.resize(get_open_max());
    for (auto &ch : channels_) {
        ch = nullptr;
    }
}

// 析构函数必须在此定义,否则会导致析构函数找不到EventQueue的定义
Poller::~Poller() { ; }

void Poller::init() {
    eventQueue_->setPoller(this);
    eventQueue_->start();
    timerQueue_->setPoller(this);
    timerQueue_->start();
}

void Poller::wakeup() {
    assert(eventQueue_);
    eventQueue_->wakeup();
}

TimerId Poller::runAt(const Timestamp &timestamp, const BasicHandler &handler) {
    Timer timer(timestamp, handler);
    timerQueue_->insert(timer);
    return timer.id();
}

TimerId Poller::runAfter(const Timestamp &timestamp,
                         const BasicHandler &handler) {
    return runAt(timestamp + Timestamp::now(), handler);
}

TimerId Poller::runEvery(const Timestamp &timestamp,
                         const BasicHandler &handler) {
    TimerId id = Timer::getOneValidId();
    runEveryHelper(id, timestamp, handler);
    return id;
}

#ifdef __APPLE__
void Poller::handleTimeoutEvents() { timerQueue_->handleTimeoutEvents(); }
#endif

void Poller::runEveryHelper(TimerId id, const Timestamp &timestamp,
                            const BasicHandler &handler) {
    Timer timer(id, timestamp + Timestamp::now(),
                [handler, timestamp, this, id] {
                    runEveryHelper(id, timestamp, handler);
                    handler();
                });
    timerQueue_->insert(timer);
}

void Poller::cancelTimer(TimerId id) { timerQueue_->cancel(id); }

void Poller::cancelAllTimer() { timerQueue_->cancelAll(); }

void Poller::runEventHandler(const BasicHandler &handler) {
    eventQueue_->pushHandler(handler);
    wakeup();
}

#ifdef __APPLE__
Timestamp Poller::getMinTimestamp() { return timerQueue_->getMinTimestamp(); }
#endif

PollerEpoll::PollerEpoll() {
#ifdef __linux__
    pollerfd_ = ::epoll_create1(EPOLL_CLOEXEC);
    fatalif(pollerfd_ <= 0, "%s", ::strerror(errno));
    pollerfp_ = std::make_shared<FileDescriptor>(pollerfd_);

    events_.resize(defaultEpollSize);
#endif
}

void PollerEpoll::loop_once() {
#ifdef __linux__
    int nfds = ::epoll_wait(pollerfd_, getEventsPtr(), events_.size(), -1);
    verbose("nfds = %d events_.size() = %u", nfds, events_.size());
    for (int i = 0; i < nfds; i++) {
        int fd = events_[i].data.fd;
        Channel *ch = getChannel(fd);
        // verbose("event in chan %p", ch);
        if (ch == NULL)
            continue;
        int revent = 0;
        if (events_[i].events & EPOLLIN)
            revent |= Channel::kRead_;
        if (events_[i].events & EPOLLOUT)
            revent |= Channel::kWrit_;
        if (events_[i].events & (EPOLLRDHUP | EPOLLERR))
            revent |= Channel::kErro_;
        ch->updateRevents(revent);
        ch->handleEvent();
    }
    if (eventsSize_ > events_.size()) {
        events_.resize(eventsSize_ * 2);
    }
#endif
}

#ifdef __linux__
void PollerEpoll::translateEvents(struct epoll_event &event, Channel *chan) {
    assert(chan);
    //    event.data.ptr = reinterpret_cast<void *>(chan);
    event.data.fd = chan->getChanelFd();
    event.events = EPOLLHUP;
    if (chan->getEvents() & Channel::kWrit_)
        event.events |= EPOLLOUT;
    if (chan->getEvents() & Channel::kRead_)
        event.events |= EPOLLIN;
}
#endif

void PollerEpoll::addChanel(Channel *chan) {
#ifdef __linux__
    assert(chan && chan->getChanelFd() >= 0);

    struct epoll_event event;
    translateEvents(event, chan);

    verbose("add fd = %d, chan = %p\n", chan->getChanelFd(), chan);

    setChannel(chan->getChanelFd(), chan);

    int ret =
        ::epoll_ctl(pollerfd_, EPOLL_CTL_ADD, chan->getChanelFd(), &event);
    if (ret < 0)
        throw BaseException(::strerror(errno));

    eventsSize_++;
#endif
}

void PollerEpoll::removeChanel(Channel *chan) {
#ifdef __linux__
    assert(chan && chan->getChanelFd() >= 0);

    struct epoll_event event = {.events = 0, .data = {.ptr = NULL}};

    verbose("remove fd = %d, chan = %p\n", chan->getChanelFd(), chan);

    setChannel(chan->getChanelFd(), nullptr);

    ::epoll_ctl(pollerfd_, EPOLL_CTL_MOD, chan->getChanelFd(), &event);
    int ret =
        ::epoll_ctl(pollerfd_, EPOLL_CTL_DEL, chan->getChanelFd(), &event);
    if (ret < 0)
        throw BaseException(::strerror(errno));

    eventsSize_--;
#endif
}

void PollerEpoll::updateChanel(Channel *chan) {
#ifdef __linux__
    assert(chan && chan->getChanelFd() >= 0);

    if (!chan->isEventsChanged()) {
        return;
    }

    struct epoll_event event;
    translateEvents(event, chan);

    int ret =
        ::epoll_ctl(pollerfd_, EPOLL_CTL_MOD, chan->getChanelFd(), &event);
    if (ret < 0)
        throw BaseException(::strerror(errno));
#endif
}

PollerSelect::PollerSelect() {
    FD_ZERO(&rset_);
    FD_ZERO(&wset_);
}

void PollerSelect::loop_once() {
    struct timeval *pto = nullptr;
#ifdef __APPLE__
    struct timeval timeout;
    Timestamp min = getMinTimestamp();
    if (!min.invalid()) {
        Timestamp now = Timestamp::now();
        if (min < now) {
            timeout = {0, 0};
        } else {
            Timestamp inter = min - now;
            ClearUnuseVariableWarning(inter);
            timeout = {.tv_sec = min.sec(), .tv_usec = min.usec()};
        }
        pto = &timeout;
    }
#endif

    fd_set rset = rset_;
    fd_set wset = wset_;

    int ready = ::select(maxfd_, &rset, &wset, NULL, pto);
    errorif(ready == -1, "select: %s\n", ::strerror(errno));

#ifdef __APPLE__
    // select will return zero when timeout expires
    handleTimeoutEvents();
#endif

    for (int fd = 0; fd < maxfd_ && ready > 0; fd++) {
        bool flag = false;
        int revent = 0;
        if (FD_ISSET(fd, &rset)) {
            flag = true;
            revent |= Channel::kRead_;
        }
        if (FD_ISSET(fd, &wset)) {
            flag = true;
            revent |= Channel::kWrit_;
        }
        if (flag) {
            Channel *ch = getChannel(fd);
            verbose("loop onct in fd %d chanel %p", fd, getChannel(fd));
            ready--;

            if (ch == nullptr)
                continue;
            ch->updateRevents(revent);
            ch->handleEvent();
        }
    }
}

void PollerSelect::addChanel(Channel *chan) {
    assert(chan && chan->getChanelFd() >= 0);

    auto fd = chan->getChanelFd();
    if (fd >= FD_SETSIZE) {
        throw BaseException("fd >= 1024");
    }

    verbose("try to add fd %d chanel %p", fd, chan);

    if (fd >= maxfd_) {
        maxfd_ = fd + 1;
    }
    setChannel(fd, chan);

    auto event = chan->getEvents();
    if (event & Channel::kRead_) {
        FD_SET(fd, &rset_);
    }
    if (event & Channel::kWrit_) {
        FD_SET(fd, &wset_);
    }
}

void PollerSelect::updateChanel(Channel *chanel) {
    assert(chanel && chanel->getChanelFd() >= 0);

    if (!chanel->isEventsChanged()) {
        return;
    }

    verbose("update chanel for %p", chanel);

    auto fd = chanel->getChanelFd();
    auto event = chanel->getEvents();
    if ((event & Channel::kRead_) && (!FD_ISSET(fd, &rset_))) {
        FD_SET(fd, &rset_);
    }
    if (!(event & Channel::kRead_) && (FD_ISSET(fd, &rset_))) {
        FD_CLR(fd, &rset_);
    }
    if ((event & Channel::kWrit_) && (!FD_ISSET(fd, &wset_))) {
        FD_SET(fd, &wset_);
    }
    if (!(event & Channel::kWrit_) && (FD_ISSET(fd, &wset_))) {
        FD_CLR(fd, &wset_);
    }
}

void PollerSelect::removeChanel(Channel *chan) {
    assert(chan && chan->getChanelFd() >= 0);

    auto fd = chan->getChanelFd();
    FD_CLR(fd, &rset_);
    FD_CLR(fd, &wset_);

    verbose("try to remove fd %d chan %p", fd, chan);

    if (fd == maxfd_) {
        while (--maxfd_) {
            if (FD_ISSET(maxfd_, &rset_) || FD_ISSET(maxfd_, &wset_))
                break;
        }
    }

    setChannel(fd, nullptr);
}

PollerPoll::PollerPoll() { pollfds_.resize(defaultPollSize); }

void PollerPoll::loop_once() {
    int interMs = -1;

#ifdef __APPLE__
    Timestamp min = getMinTimestamp();
    if (!min.invalid()) {
        Timestamp now = Timestamp::now();
        if (min < now) {
            interMs = 0;
        } else {
            Timestamp inter = min - now;
            interMs = static_cast<int>(inter.toMillSec());
        }
    }
#endif
    int ready = ::poll(getPollfdPtr(), pollfds_.size(), interMs);

#ifdef __APPLE__
    // poll return zero when interMs expires
    handleTimeoutEvents();
#endif

    for (uint32_t i = 0; i < pollfds_.size() && ready > 0; i++) {
        int event = 0;
        int fd = pollfds_[i].fd;
        int revent = pollfds_[i].revents;
#ifdef __linux__
        if (revent & POLLRDHUP) {
            event |= Channel::kErro_;
        }
#endif
        if (revent & POLLERR) {
            event |= Channel::kErro_;
        }
        if (revent & POLLIN) {
            event |= Channel::kRead_;
        }
        if (revent & POLLOUT) {
            event |= Channel::kWrit_;
        }

        if (event == 0)
            continue;
        ready--;

        Channel *ch = getChannel(fd);
        if (ch == nullptr)
            continue;
        ch->updateRevents(event);
        ch->handleEvent();
    }
}

void PollerPoll::addChanel(Channel *chan) {
    assert(chan && chan->getChanelFd() >= 0);

    int fd = chan->getChanelFd();
    int event = chan->getEvents();

    struct pollfd pollfd_;
    pollfd_.fd = fd;
#ifdef __linux__
    pollfd_.events = POLLRDHUP | POLLERR;
#elif defined(__APPLE__)
    pollfd_.events = POLLERR | POLLHUP;
#endif
    if (event & Channel::kRead_)
        pollfd_.events |= POLLIN;
    if (event & Channel::kWrit_)
        pollfd_.events |= POLLOUT;

    if (pollfds_.size() <= static_cast<decltype(pollfds_.size())>(fd)) {
        pollfds_.resize(fd + 1);
    }
    pollfds_[fd] = pollfd_;
    setChannel(fd, chan);
}

void PollerPoll::updateChanel(Channel *chan) {
    assert(chan && chan->getChanelFd() >= 0);

    if (!chan->isEventsChanged()) {
        return;
    }

    int fd = chan->getChanelFd();
    int event = chan->getEvents();
    struct pollfd *p = &pollfds_[fd];

    if (event & Channel::kRead_) {
        if (!(p->events & POLLIN)) {
            p->events |= POLLIN;
        }
    } else {
        if (p->events & POLLIN) {
            p->events &= ~POLLIN;
        }
    }
    if (event & Channel::kWrit_) {
        if (!(p->events & POLLOUT)) {
            p->events |= POLLOUT;
        }
    } else {
        if (p->events & POLLOUT) {
            p->events &= ~POLLOUT;
        }
    }
}

void PollerPoll::removeChanel(Channel *chan) {
    assert(chan && chan->getChanelFd() >= 0 && pollfds_.size() > 0);

    int fd = chan->getChanelFd();
    setChannel(fd, nullptr);
    pollfds_[fd].fd = -1;
}

PollerKevent::PollerKevent() {
#ifdef __APPLE__

    kq_ = kqueue();
    keventPtr_ = std::make_shared<FileDescriptor>(kq_);

    events_.resize(48);
    changes_.reserve(48);
#endif
}

#ifdef __APPLE__
void PollerKevent::updateKevents() {
    if (!changes_.empty()) {
        ::kevent(kq_, &changes_[0], changes_.size(), NULL, 0, NULL);
        changes_.clear();
    }
}

#endif

void PollerKevent::addChanel(Channel *ch) {
#ifdef __APPLE__
    assert(ch && ch->getChanelFd() >= 0);
    verbose("add chanel %p", ch);
    struct kevent changes;
    int events = ch->getEvents();
    if (events & Channel::kRead_) {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0,
               0, ch);
        changes_.push_back(changes);
    }
    if (events & Channel::kWrit_) {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0,
               0, ch);
        changes_.push_back(changes);
    }
    eventsSize_++;
    updateKevents();
#endif
}

void PollerKevent::removeChanel(Channel *ch) {
#ifdef __APPLE__
    assert(ch && ch->getChanelFd() >= 0);

    verbose("try to remove Channel %p", ch);

    struct kevent changes;
    static int i = 0;
    i++;
    EV_SET(&changes, ch->getChanelFd(), EVFILT_READ, EV_DELETE, 0, 0, NULL);
    int k1 = ::kevent(kq_, &changes, 1, NULL, 0, NULL);
    ClearUnuseVariableWarning(k1);
    EV_SET(&changes, ch->getChanelFd(), EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    int k2 = ::kevent(kq_, &changes, 1, NULL, 0, NULL);
    ClearUnuseVariableWarning(k2);
    eventsSize_--;
    changes_.erase(std::remove_if(changes_.begin(), changes_.end(),
                                  [ch](struct kevent &ke) -> bool {
                                      return ke.udata ==
                                             static_cast<decltype(ke.udata)>(
                                                 ch);
                                  }),
                   changes_.end());
#endif
}

void PollerKevent::updateChanel(Channel *ch) {
#ifdef __APPLE__
    if (!ch->isEventsChanged()) {
        // return;
    }

    verbose("try to update %p", ch);

    struct kevent changes;
    int events = ch->getEvents();
    if (events & Channel::kRead_) {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0,
               0, ch);
    } else {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_READ, EV_DELETE, 0, 0, ch);
    }
    changes_.push_back(changes);
    if (events & Channel::kWrit_) {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0,
               0, ch);
    } else {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_WRITE, EV_DELETE, 0, 0, ch);
    }
    changes_.push_back(changes);
    updateKevents();
#endif
}

void PollerKevent::loop_once() {
#ifdef __APPLE__
    struct timespec timeout;
    struct timespec *ptimeout = nullptr;
    Timestamp min = getMinTimestamp();
    Timestamp now = Timestamp::now();
    if (!(min < now)) {
        Timestamp inter = min - now;
        timeout = {.tv_sec = inter.sec(), .tv_nsec = inter.usec() * 1000};
        ptimeout = &timeout;
    }

    if (ptimeout)
        verbose("timeout: sec = %lu, nsec = %lu", ptimeout->tv_sec,
                ptimeout->tv_nsec);

    int ready = ::kevent(kq_, NULL, 0, &events_[0], events_.size(), ptimeout);

    handleTimeoutEvents();

    for (int i = 0; i < ready; i++) {
        struct kevent &ke = events_[i];
        Channel *chan = reinterpret_cast<Channel *>(ke.udata);
        if (chan) {
            int events = 0;
            if (ke.flags & EV_EOF || ke.flags & EV_ERROR) {
                events |= Channel::kErro_;
            }
            if (ke.filter == EVFILT_READ) {
                events |= Channel::kRead_;
            }
            if (ke.filter == EVFILT_WRITE) {
                events |= Channel::kWrit_;
            }
            verbose("loop once in chanel %p", chan);
            chan->updateRevents(events);
            chan->handleEvent();
        }
    }
    updateKevents();
    if (eventsSize_ > events_.size()) {
        events_.resize(eventsSize_ * 2);
    }
#endif
}
