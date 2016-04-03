#include "Connection.h"
#include "Buffer.h"
#include "Chanel.h"
#include "Poller.h"
#include "string.h"

using namespace Liby;

Connection::Connection(Poller *poller, std::shared_ptr<File> &fp) {
    assert(poller && fp && fp->fd() >= 0);
    poller_ = poller;
    chan_ = std::make_unique<Chanel>(poller, fp);
    readBuf_ = std::make_unique<Buffer>(1024);
    writBuf_ = std::make_unique<Buffer>(1024);
}

void Connection::init() {
    chan_->setReadEventCallback(std::bind(&Connection::onRead, this));
    chan_->setWritEventCallback(std::bind(&Connection::onWrit, this));
    chan_->setErroEventCallback(std::bind(&Connection::onErro, this));
    chan_->enableRead();
    chan_->addChanel();
}

void Connection::onRead() {
    int ret = chan_->filePtr()->read(*readBuf_);
    if (ret > 0) {
        if (readEventCallback_) {
            readEventCallback_(shared_from_this());
        }
    } else {
        if (ret != 0 && errno == EAGAIN) {
            return;
        } else {
            if (errnoEventCallback_) {
                errnoEventCallback_(shared_from_this());
            }
        }
    }
}

void Connection::onWrit() {
    if (writBuf_->size() > 0) {
        int ret = chan_->filePtr()->write(*writBuf_);
        if (ret > 0) {
            if (writBuf_->size() == 0) {
                if (writeAllCallback_) {
                    writeAllCallback_(shared_from_this());
                }
                // chan_->enableWrit(false);
                // chan_->updateChanel();
            }
        } else {
            if (ret != 0 && errno == EAGAIN) {
                return;
            } else {
                if (errnoEventCallback_) {
                    errnoEventCallback_(shared_from_this());
                }
            }
        }
    } else {
        chan_->enableWrit(false);
        chan_->updateChanel();
    }
}

void Connection::onErro() {
    if (errnoEventCallback_) {
        errnoEventCallback_(shared_from_this());
    }
}

void Connection::setReadCallback(ConnCallback cb) { readEventCallback_ = cb; }

void Connection::setWritCallback(ConnCallback cb) { writeAllCallback_ = cb; }

void Connection::setErroCallback(ConnCallback cb) { errnoEventCallback_ = cb; }

Poller *Connection::getPoller() const { return poller_; }

void Connection::runEventHandler(BasicHandler handler) {
    assert(poller_);
    poller_->runEventHandler(handler);
}

void Connection::destroy() {
    chan_.reset();
    readBuf_.reset();
    writBuf_.reset();
    cancelAllTimer();
    readEventCallback_ = nullptr;
    writeAllCallback_ = nullptr;
    errnoEventCallback_ = nullptr;
}

int Connection::getConnfd() const { return chan_->filePtr()->fd(); }

void Connection::send(const char *buf, off_t len) {
    writBuf_->append(buf, len);
    chan_->enableWrit();
    chan_->updateChanel();
}

void Connection::send(Buffer &buf) {
    writBuf_->append(buf);
    buf.retrieve();
    chan_->enableWrit();
    chan_->updateChanel();
}

void Connection::send(Buffer &&buf) {
    writBuf_->append(buf);
    buf.retrieve();
    chan_->enableWrit();
    chan_->updateChanel();
}

void Connection::send(const char *buf) {
    assert(buf);
    send(buf, ::strlen(buf));
}

void Connection::send(const char c) { writBuf_->append(&c, 1); }

Buffer &Connection::read() { return *readBuf_; }

void Connection::suspendRead(bool flag) {
    if (flag) {
        chan_->enableRead(false);
    } else {
        chan_->enableRead(true);
    }
    chan_->updateChanel();
}

void Connection::suspendWrit(bool flag) {
    if (flag) {
        chan_->enableWrit(false);
    } else {
        chan_->enableWrit(true);
    }
    chan_->updateChanel();
}

TimerId Connection::runAt(const Timestamp &timestamp,
                          const BasicHandler &handler) {
    TimerId id = poller_->runAt(timestamp, handler);
    timerIds_.insert(id);
    return id;
}

TimerId Connection::runAfter(const Timestamp &timestamp,
                             const BasicHandler &handler) {
    TimerId id = poller_->runAfter(timestamp, handler);
    timerIds_.insert(id);
    return id;
}

TimerId Connection::runEvery(const Timestamp &timestamp,
                             const BasicHandler &handler) {
    TimerId id = poller_->runEvery(timestamp, handler);
    timerIds_.insert(id);
    return id;
}

void Connection::cancelAllTimer() {
    for (auto &x : timerIds_) {
        poller_->cancelTimer(x);
    }
    timerIds_.clear();
}

void Connection::cancelTimer(TimerId id) {
    timerIds_.erase(id);
    poller_->cancelTimer(id);
}
