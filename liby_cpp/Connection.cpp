#include "Connection.h"

void Connection::init() {
    chan_->setReadEventCallback(std::bind(&Connection::handleReadEvent, this));
    chan_->setWritEventCallback(std::bind(&Connection::handleWritEvent, this));
    chan_->setErroEventCallback(std::bind(&Connection::handleErroEvent, this));
}

void Connection::handleReadEvent() {
    DeferCaller defer([this] { updateConnectionState(); });
    while (!readTaskQueue_.empty()) {
        auto task = readTaskQueue_.front();
        int ret = chan_->fileHandlePtr()->read(task.buf_ + task.offset_,
                                               task.length_ - task.offset_);
        if (ret > 0) {
            task.offset_ += ret;
            if (task.offset_ >= task.min_except_bytes) {
                if (task.handler_) {
                    task.handler_(task.buf_, task.offset_, 0);
                }
                readTaskQueue_.pop();
            }
        } else {
            if (ret != 0 && errno == EAGAIN) {
                return;
            } else {
                int ec = errno;
                if (errno == 0 && ret == 0)
                    ec = -1;
                if (task.handler_) {
                    task.handler_(task.buf_, task.offset_, ec);
                }
                readTaskQueue_.pop();
                defer.cancel();
                handleErroEvent();
            }
        }
    }
}

void Connection::handleWritEvent() {
    DeferCaller defer([this] { updateConnectionState(); });
    while (!writTaskQueue_.empty()) {
        auto task = writTaskQueue_.front();
        int ret = chan_->fileHandlePtr()->write(task.buf_ + task.offset_,
                                                task.length_ - task.offset_);
        if (ret > 0) {
            task.offset_ += ret;
            if (task.offset_ >= task.min_except_bytes) {
                if (task.handler_) {
                    task.handler_(task.buf_, task.offset_, 0);
                }
                writTaskQueue_.pop();
            }
        } else {
            if (ret != 0 && errno == EAGAIN) {
                return;
            } else {
                int ec = errno;
                if (errno == 0 && ret == 0)
                    ec = -1;
                if (task.handler_) {
                    task.handler_(task.buf_, task.offset_, ec);
                }
                writTaskQueue_.pop();
                defer.cancel();
                handleErroEvent();
            }
        }
    }
}

void Connection::handleErroEvent() {
    int savedErrno = errno == 0 ? -1 : errno;
    while (!readTaskQueue_.empty()) {
        auto task = readTaskQueue_.pop_front();
        if (task.handler_) {
            task.handler_(task.buf_, task.offset_, savedErrno);
        }
    }
    while (!writTaskQueue_.empty()) {
        auto task = writTaskQueue_.pop_front();
        if (task.handler_) {
            task.handler_(task.buf_, task.offset_, savedErrno);
        }
    }
}

void Connection::setPoller(Poller *poller) {
    chan_->setEvents(EPOLLERR | EPOLLRDHUP | EPOLLHUP);
    chan_->setPoller(poller);
    chan_->addChanelToPoller();
}

void Connection::updateConnectionState() {
    chan_->enableRead(!readTaskQueue_.empty());
    chan_->enableWrit(!writTaskQueue_.empty());
    chan_->flushChanelState();
}

void Connection::async_read_some(char *buf, off_t buffersize,
                                 off_t min_except_bytes, io_handler handler) {
    io_task task;
    task.buf_ = buf;
    task.length_ = buffersize;
    task.handler_ = handler;
    readTaskQueue_.push_back(task);
}

void Connection::async_writ_some(char *buf, off_t buffersize,
                                 off_t min_except_bytes, io_handler handler) {
    io_task task;
    task.buf_ = buf;
    task.length_ = buffersize;
    task.handler_ = handler;
    writTaskQueue_.push_back(task);
}
