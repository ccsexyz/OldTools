#include "Connection.h"
#include <string.h>

using namespace Liby;

Connection::Connection(Poller *poller, std::shared_ptr<File> &fp) : TimerSet(poller){
    assert(poller && fp && fp->fd() >= 0);
    poller_ = poller;
    chan_ = std::make_unique<Chanel>(poller, fp);
    readBuf_ = std::make_unique<Buffer>(4096);
//    writBuf_ = std::make_unique<Buffer>(4096);
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
            auto x = shared_from_this(); // readEventCallback可能会试图销毁这个对象
                                         // 在这里给引用计数加一以适当延长对象生命期
                                         // 从readEventCallback返回时可能对象内部的Chanel,Buffer对象已经失效
            readEventCallback_(x);
            if(!readBuf_)
                return;
//            if (!writBuf_)
//                return;
            if (writBytes_ >= writableLimit_) {
                if (writableLimitCallback_) {
                    writableLimitCallback_(shared_from_this());
                } else {
                    chan_->enableRead(false);
                }
            }
            if (chan_) {
                chan_->updateChanel();
            }
        }
    } else {
        if (ret != 0 && errno == EAGAIN) {
            return;
        } else {
            onErro();
        }
    }
}

void Connection::onWrit() {
    if (writTasks_.empty()) {
        chan_->enableWrit(false);
        return;
    }
    int ret = chan_->filePtr()->write(writTasks_);
    if (ret > 0) {
        writBytes_ -= ret;
        if (writTasks_.empty()) {
            chan_->enableRead();
            chan_->enableWrit(false);
            if (writeAllCallback_) {
                auto x = shared_from_this();
                writeAllCallback_(x);
                if (!chan_)
                    return;
            }
            chan_->updateChanel();
        }
    } else {
        if (ret != 0 && errno == EAGAIN) {
            return;
        } else {
            onErro();
        }
    }
}

void Connection::onErro() {
    auto conn = shared_from_this();
    if (errnoEventCallback_) {
        errnoEventCallback_((conn));
    }
    destroy();
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
    debug("try to destroy connection %p", this);
    chan_.reset();
    readBuf_.reset();
    writTasks_.clear();
    cancelAllTimer();
    writeAllCallback_ = nullptr;
    errnoEventCallback_ = nullptr;
    readEventCallback_ = nullptr;
}

int Connection::getConnfd() const { return chan_->filePtr()->fd(); }

void Connection::send(const char *buf, off_t len) {
    if(!readBuf_)
        return;
    io_task task;
    task.buffer_ = std::make_shared<Buffer>(buf, len);
    writTasks_.emplace_back(task);
    chan_->enableWrit();
    chan_->updateChanel();
    writBytes_ += len;
}

void Connection::send(Buffer &buf) {
    if(!readBuf_)
        return;
    writBytes_ += buf.size();
    io_task task;
    task.buffer_ = std::make_shared<Buffer>(buf.capacity());
    task.buffer_->swap(buf);
    writTasks_.emplace_back(task);
    chan_->enableWrit();
    chan_->updateChanel();
}

void Connection::send(Buffer &&buf) {
    if(!readBuf_)
        return;
    writBytes_ += buf.size();
    io_task task;
    task.buffer_ = std::make_shared<Buffer>(std::move(buf));
    writTasks_.emplace_back(task);
    chan_->enableWrit();
    chan_->updateChanel();
}

void Connection::send(const char *buf) {
    assert(buf);
    send(buf, ::strlen(buf));
}

void Connection::send(const char c) { send(&c, 1); }

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

//TimerId Connection::runAt(const Timestamp &timestamp,
//                          const BasicHandler &handler) {
//    TimerId id = poller_->runAt(timestamp, handler);
//    timerIds_.insert(id);
//    return id;
//}
//
//TimerId Connection::runAfter(const Timestamp &timestamp,
//                             const BasicHandler &handler) {
//    TimerId id = poller_->runAfter(timestamp, handler);
//    timerIds_.insert(id);
//    return id;
//}
//
//TimerId Connection::runEvery(const Timestamp &timestamp,
//                             const BasicHandler &handler) {
//    TimerId id = poller_->runEvery(timestamp, handler);
//    timerIds_.insert(id);
//    return id;
//}
//
//void Connection::cancelAllTimer() {
//    for (auto &x : timerIds_) {
//        poller_->cancelTimer(x);
//    }
//    timerIds_.clear();
//}
//
//void Connection::cancelTimer(TimerId id) {
//    timerIds_.erase(id);
//    poller_->cancelTimer(id);
//}

void Connection::setWritableLimit(size_t writableLimit) {
    writableLimit_ = writableLimit;
}

void Connection::setWritLimitCallback(ConnCallback cb) {
    writableLimitCallback_ = cb;
}

void Connection::setErro() {
    if (chan_) {
        send(" ");
        chan_->enableErro();
    }
}

void Connection::sendFile(std::shared_ptr<File> fp, off_t offset, off_t len) {
    assert(fp && offset >= 0 && len >= 0);

    io_task task;
    task.fp_ = fp;
    task.offset_ = offset;
    task.len_ = len;
    writTasks_.push_back(task);
    auto t = writTasks_.front();
    chan_->enableWrit();
    chan_->updateChanel();
}

Connection::~Connection() {
    debug("DELETE CONNECTION %p", this);
}


