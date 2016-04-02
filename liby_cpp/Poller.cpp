#include "Poller.h"
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

PollerEpoll::PollerEpoll() {
#ifdef __linux__
    pollerfd_ = ::epoll_create1(EPOLL_CLOEXEC);
    // fatalif(pollerfd_ <= 0, "%s", ::strerror(errno));
    pollerfp_ = std::make_shared<File>(pollerfd_);

    eventfd_ = ::eventfd(1000, EFD_CLOEXEC | EFD_NONBLOCK);
    // fatalif(eventfd_ <= 0, "%s", ::strerror(errno));
    eventfp_ = std::make_shared<File>(eventfd_);

    eventChanelPtr_ = std::make_shared<Chanel>(this, eventfp_);
    eventHandlersPtr_ = std::make_shared<BlockingQueue<BasicHandler>>();

    events_.resize(defaultEpollSize);

    initEventfd();
#endif
}

#ifdef __linux__
void PollerEpoll::initEventfd() {
    eventChanelPtr_->enableRead();
    eventChanelPtr_->setReadEventCallback([this] {
        if (!eventfp_)
            return;
        char buf[128];
        int ret = eventfp_->read(buf, 128);
        if (ret <= 0)
            return;

        while (!eventHandlersPtr_->empty()) {
            auto f_ = eventHandlersPtr_->pop_front();
            f_();
        }
    });
    eventChanelPtr_->setErroEventCallback([this] {
        eventChanelPtr_.reset();
        eventHandlersPtr_.reset();
    });
    addChanel(eventChanelPtr_.get());
}
#endif

void PollerEpoll::loop_once(int interMs) {
#ifdef __linux__
    int nfds = ::epoll_wait(pollerfd_, getEventsPtr(), events_.size(), interMs);
    for (int i = 0; i < nfds; i++) {
        Chanel *ch = reinterpret_cast<Chanel *>(events_[i].data.ptr);
        debug("event in chan %p", ch);
        if (ch == NULL)
            continue;
        int revent = 0;
        if (events_[i].events & EPOLLIN)
            revent |= Chanel::kRead_;
        if (events_[i].events & EPOLLOUT)
            revent |= Chanel::kWrit_;
        if (events_[i].events & (EPOLLRDHUP | EPOLLERR))
            revent |= Chanel::kErro_;
        ch->updateRevents(revent);
        ch->handleEvent();
    }
    if (eventsSize_ > events_.size()) {
        events_.resize(eventsSize_ * 2);
    }
#endif
}

#ifdef __linux__
void PollerEpoll::translateEvents(struct epoll_event &event, Chanel *chan) {
    assert(chan);
    event.data.ptr = reinterpret_cast<void *>(chan);
    event.events = EPOLLHUP;
    if (chan->getEvents() & Chanel::kWrit_)
        event.events |= EPOLLOUT;
    if (chan->getEvents() & Chanel::kRead_)
        event.events |= EPOLLIN;
}
#endif

void PollerEpoll::addChanel(Chanel *chan) {
#ifdef __linux__
    assert(chan && chan->getChanelFd() >= 0);

    struct epoll_event event;
    translateEvents(event, chan);

    debug("add fd = %d, chan = %p\n", chan->getChanelFd(), chan);

    int ret =
        ::epoll_ctl(pollerfd_, EPOLL_CTL_ADD, chan->getChanelFd(), &event);
    if (ret < 0)
        throw BaseException(__FILE__, __LINE__, ::strerror(errno));

    eventsSize_++;
#endif
}

void PollerEpoll::removeChanel(Chanel *chan) {
#ifdef __linux__
    assert(chan && chan->getChanelFd() >= 0);

    struct epoll_event event = {.events = 0, .data = {.ptr = NULL}};

    debug("remove fd = %d, chan = %p\n", chan->getChanelFd(), chan);

    ::epoll_ctl(pollerfd_, EPOLL_CTL_MOD, chan->getChanelFd(), &event);
    int ret =
        ::epoll_ctl(pollerfd_, EPOLL_CTL_DEL, chan->getChanelFd(), &event);
    if (ret < 0)
        throw BaseException(__FILE__, __LINE__, ::strerror(errno));

    eventsSize_--;
#endif
}

void PollerEpoll::updateChanel(Chanel *chan) {
#ifdef __linux__
    assert(chan && chan->getChanelFd() >= 0);

    struct epoll_event event;
    translateEvents(event, chan);

    int ret =
        ::epoll_ctl(pollerfd_, EPOLL_CTL_MOD, chan->getChanelFd(), &event);
    if (ret < 0)
        throw BaseException(__FILE__, __LINE__, ::strerror(errno));
#endif
}

void PollerEpoll::runEventHandler(BasicHandler handler) {
#ifdef __linux__
    if (eventChanelPtr_ && eventfd_ >= 0) {
        eventHandlersPtr_->push_back(handler);
        wakeup();
    } else {
        error("Cannot run EventHandler");
    }
#endif
}

void PollerEpoll::wakeup() {
    static int64_t this_is_a_number = 1;
    eventfp_->write(&this_is_a_number, sizeof(this_is_a_number));
}

PollerSelect::PollerSelect() {
#ifdef __linux__
    eventfd_ = ::eventfd(1000, EFD_CLOEXEC | EFD_NONBLOCK);
    // fatalif(eventfd_ <= 0, "%s", ::strerror(errno));
    eventfp_ = std::make_shared<File>(eventfd_);
#elif defined(__APPLE__)
    int fildes[2];
    ::pipe(fildes);
    eventfd_ = fildes[0];
    eventfp_ = std::make_shared<File>(eventfd_);
    eventfd2_ = fildes[1];
    eventfp2_ = std::make_shared<File>(eventfd2_);
#endif

    eventChanelPtr_ = std::make_shared<Chanel>(this, eventfp_);
    eventHandlersPtr_ = std::make_shared<BlockingQueue<BasicHandler>>();

    chanels_.resize(48);
    FD_ZERO(&rset_);
    FD_ZERO(&wset_);

    initEventfd();
}

void PollerSelect::initEventfd() {
    eventChanelPtr_->enableRead();
    eventChanelPtr_->setReadEventCallback([this] {
        if (!eventfp_)
            return;
        char buf[128];
        int ret = eventfp_->read(buf, 128);
        if (ret <= 0)
            return;

        while (!eventHandlersPtr_->empty()) {
            auto f_ = eventHandlersPtr_->pop_front();
            f_();
        }
    });
    eventChanelPtr_->setErroEventCallback([this] {
        eventChanelPtr_.reset();
        eventHandlersPtr_.reset();
    });
    addChanel(eventChanelPtr_.get());
}

void PollerSelect::loop_once(int interMs) {
    struct timeval timeout;
    struct timeval *pto;
    if (interMs >= 0) {
        timeout.tv_sec = interMs / 1000;
        interMs %= 1000;
        timeout.tv_usec = interMs * 1000;
        pto = &timeout;
    } else {
        pto = nullptr;
    }

    fd_set rset = rset_;
    fd_set wset = wset_;

    int ready = ::select(maxfd_, &rset, &wset, NULL, pto);
    errorif(ready == -1, "select: %s\n", ::strerror(errno));

    for (int fd = 0; fd < maxfd_ && ready > 0; fd++) {
        bool flag = false;
        int revent = 0;
        if (FD_ISSET(fd, &rset)) {
            flag = true;
            revent |= Chanel::kRead_;
        }
        if (FD_ISSET(fd, &wset)) {
            flag = true;
            revent |= Chanel::kWrit_;
        }
        if (flag) {
            ready--;
            chanels_[fd]->updateRevents(revent);
            chanels_[fd]->handleEvent();
        }
    }
}

void PollerSelect::addChanel(Chanel *chan) {
    assert(chan && chan->getChanelFd() >= 0);

    auto fd = chan->getChanelFd();
    if (fd >= FD_SETSIZE) {
        throw BaseException(__FILE__, __LINE__, "fd >= 1024");
    }

    if (fd >= maxfd_) {
        maxfd_ = fd + 1;
        if (maxfd_ > chanels_.size())
            chanels_.resize(maxfd_);
    }
    chanels_[fd] = chan;

    auto event = chan->getEvents();
    if (event & Chanel::kRead_) {
        FD_SET(fd, &rset_);
    }
    if (event & Chanel::kWrit_) {
        FD_SET(fd, &wset_);
    }
}

void PollerSelect::updateChanel(Chanel *chanel) {
    assert(chanel && chanel->getChanelFd() >= 0);

    if (!chanel->isEventsChanged()) {
        return;
    }

    auto fd = chanel->getChanelFd();
    auto event = chanel->getEvents();
    if ((event & Chanel::kRead_) && (!FD_ISSET(fd, &rset_))) {
        FD_SET(fd, &rset_);
    }
    if (!(event & Chanel::kRead_) && (FD_ISSET(fd, &rset_))) {
        FD_CLR(fd, &rset_);
    }
    if ((event & Chanel::kWrit_) && (!FD_ISSET(fd, &wset_))) {
        FD_SET(fd, &wset_);
    }
    if (!(event & Chanel::kWrit_) && (FD_ISSET(fd, &wset_))) {
        FD_CLR(fd, &wset_);
    }
}

void PollerSelect::removeChanel(Chanel *chan) {
    assert(chan && chan->getChanelFd() >= 0);

    auto fd = chan->getChanelFd();
    FD_CLR(fd, &rset_);
    FD_CLR(fd, &wset_);

    if (fd == maxfd_) {
        while (--maxfd_) {
            if (FD_ISSET(maxfd_, &rset_) || FD_ISSET(maxfd_, &wset_))
                break;
        }
    }
}

void PollerSelect::runEventHandler(BasicHandler handler) {
    if (eventChanelPtr_ && eventfd_ >= 0) {
        eventHandlersPtr_->push_back(handler);
        wakeup();
    } else {
        error("Cannot run EventHandler");
    }
}

void PollerSelect::wakeup() {
    static int64_t this_is_a_number = 1;
#ifdef __linux__
    eventfp_->write(&this_is_a_number, sizeof(this_is_a_number));
#elif defined(__APPLE__)
    eventfp2_->write(&this_is_a_number, sizeof(this_is_a_number));
#endif
}

PollerPoll::PollerPoll() {
#ifdef __linux__
    eventfd_ = ::eventfd(1000, EFD_CLOEXEC | EFD_NONBLOCK);
    // fatalif(eventfd_ <= 0, "%s", ::strerror(errno));
    eventfp_ = std::make_shared<File>(eventfd_);
#elif defined(__APPLE__)
    int fildes[2];
    ::pipe(fildes);
    eventfd_ = fildes[0];
    eventfp_ = std::make_shared<File>(eventfd_);
    eventfd2_ = fildes[1];
    eventfp2_ = std::make_shared<File>(eventfd2_);
#endif

    eventChanelPtr_ = std::make_shared<Chanel>(this, eventfp_);
    eventHandlersPtr_ = std::make_shared<BlockingQueue<BasicHandler>>();

    pollfds_.resize(defaultPollSize);
    initEventfd();
}

void PollerPoll::initEventfd() {
    eventChanelPtr_->enableRead();
    eventChanelPtr_->setReadEventCallback([this] {
        if (!eventfp_)
            return;
        char buf[128];
        int ret = eventfp_->read(buf, 128);
        if (ret <= 0)
            return;

        while (!eventHandlersPtr_->empty()) {
            auto f_ = eventHandlersPtr_->pop_front();
            f_();
        }
    });
    eventChanelPtr_->setErroEventCallback([this] {
        eventChanelPtr_.reset();
        eventHandlersPtr_.reset();
    });
    addChanel(eventChanelPtr_.get());
}

void PollerPoll::loop_once(int interMs) {
    int ready = ::poll(getPollfdPtr(), pollfds_.size(), interMs);
    for (int i = 0; i < pollfds_.size() && ready > 0; i++) {
        int event = 0;
        int fd = pollfds_[i].fd;
        int revent = pollfds_[i].revents;
#ifdef __linux__
        if (revent & POLLRDHUP) {
            event |= Chanel::kErro_;
        }
#endif
        if (revent & POLLERR) {
            event |= Chanel::kErro_;
        }
        if (revent & POLLIN) {
            event |= Chanel::kRead_;
        }
        if (revent & POLLOUT) {
            event |= Chanel::kWrit_;
        }

        if (event == 0)
            continue;

        auto k_ = chanels_.find(fd);
        if (k_ == chanels_.end())
            continue;

        Chanel *chan = k_->second;
        chan->updateRevents(event);
        chan->handleEvent();
        ready--;
    }
}

void PollerPoll::addChanel(Chanel *chan) {
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
    if (event & Chanel::kRead_)
        pollfd_.events |= POLLIN;
    if (event & Chanel::kWrit_)
        pollfd_.events |= POLLOUT;

    if (pollfds_.size() <= fd) {
        pollfds_.resize(fd + 1);
    }
    pollfds_[fd] = pollfd_;
    chanels_[fd] = chan;
}

void PollerPoll::updateChanel(Chanel *chan) {
    assert(chan && chan->getChanelFd() >= 0);

    int fd = chan->getChanelFd();
    int event = chan->getEvents();
    struct pollfd *p = &pollfds_[fd];

    if (event & Chanel::kRead_) {
        if (!(p->events & POLLIN)) {
            p->events |= POLLIN;
        }
    } else {
        if (p->events & POLLIN) {
            p->events &= ~POLLIN;
        }
    }
    if (event & Chanel::kWrit_) {
        if (!(p->events & POLLOUT)) {
            p->events |= POLLOUT;
        }
    } else {
        if (p->events & POLLOUT) {
            p->events &= ~POLLOUT;
        }
    }
}

void PollerPoll::removeChanel(Chanel *chan) {
    assert(chan && chan->getChanelFd() >= 0 && pollfds_.size() > 0);

    int fd = chan->getChanelFd();
    chanels_.erase(fd);
    pollfds_[fd].fd = -1;
}

void PollerPoll::runEventHandler(BasicHandler handler) {
    if (eventChanelPtr_ && eventfd_ >= 0) {
        eventHandlersPtr_->push_back(handler);
        wakeup();
    } else {
        error("Cannot run EventHandler");
    }
}

void PollerPoll::wakeup() {
    static int64_t this_is_a_number = 1;
#ifdef __linux__
    eventfp_->write(&this_is_a_number, sizeof(this_is_a_number));
#elif defined(__APPLE__)
    eventfp2_->write(&this_is_a_number, sizeof(this_is_a_number));
#endif
}

PollerKevent::PollerKevent() {
#ifdef __APPLE__
    int fildes[2];
    ::pipe(fildes);
    eventfd_ = fildes[0];
    eventfp_ = std::make_shared<File>(eventfd_);
    eventfd2_ = fildes[1];
    eventfp2_ = std::make_shared<File>(eventfd2_);

    eventChanelPtr_ = std::make_shared<Chanel>(this, eventfp_);
    eventHandlersPtr_ = std::make_shared<BlockingQueue<BasicHandler>>();

    kq_ = kqueue();
    keventPtr_ = std::make_shared<File>(kq_);

    events_.resize(48);
    changes_.reserve(48);

    initEventfd();
#endif
}

#ifdef __APPLE__
void PollerKevent::initEventfd() {
    eventChanelPtr_->enableRead();
    eventChanelPtr_->setReadEventCallback([this] {
        if (!eventfp_)
            return;
        char buf[128];
        int ret = eventfp_->read(buf, 128);
        if (ret <= 0)
            return;

        while (!eventHandlersPtr_->empty()) {
            auto f_ = eventHandlersPtr_->pop_front();
            f_();
        }
    });
    eventChanelPtr_->setErroEventCallback([this] {
        eventChanelPtr_.reset();
        eventHandlersPtr_.reset();
    });
    addChanel(eventChanelPtr_.get());
}

void PollerKevent::updateKevents() {
    if (!changes_.empty()) {
        ::kevent(kq_, &changes_[0], changes_.size(), NULL, 0, NULL);
        changes_.clear();
    }
}

#endif

void PollerKevent::addChanel(Chanel *ch) {
#ifdef __APPLE__
    assert(ch && ch->getChanelFd() >= 0);
    struct kevent changes;
    int events = ch->getEvents();
    if (events & Chanel::kRead_) {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0,
               0, ch);
        changes_.push_back(changes);
    }
    if (events & Chanel::kWrit_) {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0,
               0, ch);
        changes_.push_back(changes);
    }
    eventsSize_++;
    updateKevents();
#endif
}

void PollerKevent::removeChanel(Chanel *ch) {
#ifdef __APPLE__
    assert(ch && ch->getChanelFd() >= 0);

    struct kevent changes;
    static int i = 0;
    i++;
    EV_SET(&changes, ch->getChanelFd(), EVFILT_READ, EV_DELETE, 0, 0, NULL);
    int k1 = ::kevent(kq_, &changes, 1, NULL, 0, NULL);
    EV_SET(&changes, ch->getChanelFd(), EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    int k2 = ::kevent(kq_, &changes, 1, NULL, 0, NULL);
    eventsSize_--;
#endif
}

void PollerKevent::updateChanel(Chanel *ch) {
#ifdef __APPLE__
    struct kevent changes;
    int events = ch->getEvents();
    if (events & Chanel::kRead_) {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_READ, EV_ADD | EV_ENABLE, 0,
               0, ch);
    } else {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_READ, EV_DELETE, 0, 0, ch);
    }
    changes_.push_back(changes);
    if (events & Chanel::kWrit_) {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0,
               0, ch);
    } else {
        EV_SET(&changes, ch->getChanelFd(), EVFILT_WRITE, EV_DELETE, 0, 0, ch);
    }
    changes_.push_back(changes);
#endif
}

void PollerKevent::loop_once(int interMs) {
#ifdef __APPLE__
    struct timespec timeout = {interMs / 1000, (interMs % 1000) * 1000000};
    struct timespec *ptimeout = NULL;
    if (interMs != -1) {
        ptimeout = &timeout;
    }
    int ready = ::kevent(kq_, NULL, 0, &events_[0], events_.size(), ptimeout);
    for (int i = 0; i < ready; i++) {
        struct kevent &ke = events_[i];
        Chanel *chan = reinterpret_cast<Chanel *>(ke.udata);
        if (chan) {
            int events = 0;
            if (ke.flags & EV_EOF || ke.flags & EV_ERROR) {
                events |= Chanel::kErro_;
            }
            if (ke.filter == EVFILT_READ) {
                events |= Chanel::kRead_;
            }
            if (ke.filter == EVFILT_WRITE) {
                events |= Chanel::kWrit_;
            }
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

void PollerKevent::runEventHandler(BasicHandler handler) {
#ifdef __APPLE__
    if (eventChanelPtr_ && eventfd_ >= 0) {
        eventHandlersPtr_->push_back(handler);
        wakeup();
    } else {
        error("Cannot run EventHandler");
    }
#endif
}

void PollerKevent::wakeup() {
#ifdef __APPLE__
    static int64_t this_is_a_number = 1;
    eventfp2_->write(&this_is_a_number, sizeof(this_is_a_number));
#endif
}
