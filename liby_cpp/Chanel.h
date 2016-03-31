#ifndef POLLERTEST_CHANEL_H
#define POLLERTEST_CHANEL_H

#include "File.h"
#include "util.h"

namespace Liby {
class Poller;

class Chanel final : clean_ {
public:
    static const int kRead_ = 0x1;
    static const int kWrit_ = 0x2;
    static const int kErro_ = 0x4;

    Chanel() = default;
    Chanel(Poller *poller, FilePtr fp) : poller_(poller), fp_(fp) {}
    ~Chanel() { removeChanel(); }

    void updateRevents(int revents) { revents_ = revents; }
    void setEvents(int events) { events_ = events; }
    int getEvents() const { return events_; }
    int getChanelFd() const {
        assert(fp_->fd() >= 0);
        return fp_->fd();
    }
    bool isEventsChanged() { return isEventsChanged_; }

    FilePtr &filePtr() { return fp_; }

    void setPoller(Poller *poller) { poller_ = poller; }
    void setFilePtr(FilePtr fp) { fp_ = fp; }
    void enableRead(bool flag = true) {
        if (flag ^ (events_ & kRead_)) {
            isEventsChanged_ = true;
            events_ = flag ? (events_ | kRead_) : (events_ & ~kRead_);
        }
    }
    void enableWrit(bool flag = true) {
        if (flag ^ (events_ & kWrit_)) {
            isEventsChanged_ = true;
            events_ = flag ? (events_ | kWrit_) : (events_ & ~kWrit_);
        }
    }

    bool isError() const { return revents_ & kErro_; }
    bool Readable() const { return revents_ & kRead_; }
    bool Writable() const { return revents_ & kWrit_; }

    void removeChanel();
    void updateChanel();
    void addChanel();

    void setReadEventCallback(const BasicHandler &cb) {
        readEventCallback_ = cb;
    }
    void setWritEventCallback(const BasicHandler &cb) {
        writEventCallback_ = cb;
    }
    void setErroEventCallback(const BasicHandler &cb) {
        erroEventCallback_ = cb;
    }

    void handleEvent();

private:
    BasicHandler readEventCallback_;
    BasicHandler writEventCallback_;
    BasicHandler erroEventCallback_;

    FilePtr fp_;
    Poller *poller_ = nullptr;
    int events_ = 0;  // update by user
    int revents_ = 0; // update by poller
    bool isEventsChanged_ = false;
};
using ChanelPtr = std::shared_ptr<Chanel>;
}

#endif // POLLERTEST_CHANEL_H
