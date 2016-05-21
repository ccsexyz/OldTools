#ifndef POLLERTEST_CHANEL_H
#define POLLERTEST_CHANEL_H

#include "FileDescriptor.h"
#include "util.h"

namespace Liby {

class Channel final : clean_ {
public:
    static const int kRead_ = 0x1;
    static const int kWrit_ = 0x2;
    static const int kErro_ = 0x4;

    Channel() = default;
    Channel(Poller *poller, FilePtr fp) : fp_(fp), poller_(poller) {}
    ~Channel();

    void updateRevents(int revents) { revents_ = revents; }
    void setEvents(int events) { events_ = events; }
    int getEvents() const { return events_; }
    int getChanelFd() const {
        assert(fp_->fd() >= 0);
        return fp_->fd();
    }
    bool isEventsChanged() { return events_ != savedEvents_; }

    FilePtr &filePtr() { return fp_; }

    Channel &setPoller(Poller *poller) {
        poller_ = poller;
        return *this;
    }
    Channel &setFilePtr(FilePtr fp) {
        fp_ = fp;
        return *this;
    }
    Channel &enableRead(bool flag = true) {
        events_ = flag ? (events_ | kRead_) : (events_ & ~kRead_);
        return *this;
    }
    Channel &enableWrit(bool flag = true) {
        events_ = flag ? (events_ | kWrit_) : (events_ & ~kWrit_);
        return *this;
    }
    Channel &enableErro(bool flag = true) {
        events_ = flag ? (events_ | kErro_) : (events_ & ~kErro_);
        return *this;
    }

    bool canRead() const { return (events_ & kRead_) != 0; }
    bool canWrit() const { return (events_ & kWrit_) != 0; }

    void removeChanel();
    void updateChanel();
    void addChanel();

    Channel &onRead(const BasicHandler &cb) {
        readEventCallback_ = cb;
        return *this;
    }
    Channel &onWrit(const BasicHandler &cb) {
        writEventCallback_ = cb;
        return *this;
    }
    Channel &onErro(const BasicHandler &cb) {
        erroEventCallback_ = cb;
        return *this;
    }

    void handleEvent();

private:
    bool isError() const { return revents_ & kErro_ || events_ & kErro_; }
    bool Readable() const { return (revents_ & kRead_) != 0; }
    bool Writable() const { return (revents_ & kWrit_) != 0; }

private:
    BasicHandler readEventCallback_;
    BasicHandler writEventCallback_;
    BasicHandler erroEventCallback_;

    FilePtr fp_;
    Poller *poller_ = nullptr;
    int events_ = 0; // update by user
    int savedEvents_ = 0;
    int revents_ = 0; // update by poller
};
using ChanelPtr = std::shared_ptr<Channel>;
}

#endif // POLLERTEST_CHANEL_H
