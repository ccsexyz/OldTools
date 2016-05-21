#include "Connection.h"
#include "Buffer.h"
#include <string.h>

#ifdef __linux__

#include <sys/eventfd.h>
#include <sys/sendfile.h>

#elif defined(__APPLE__)
#include <sys/uio.h>
#endif

using namespace Liby;

Connection::Connection(Poller *poller,
                       const std::shared_ptr<FileDescriptor> &fp)
    : TimerSet(poller) {
    assert(poller && fp && fp->fd() >= 0);
    poller_ = poller;
    chan_ = std::make_unique<Channel>(poller, fp);
    readBuf_ = std::make_unique<Buffer>(4096);
    //    writBuf_ = std::make_unique<Buffer>(4096);
}

void Connection::init() {
    self_ = shared_from_this();
    chan_->onRead([this] { handleReadEvent(); });
    chan_->onWrit([this] { handleWritEvent(); });
    chan_->onErro([this] { handleErroEvent(); });
    chan_->enableRead();
    chan_->addChanel();
}

void Connection::handleReadEvent() {
    auto ret = tryRead(chan_->getChanelFd());
    if (ret > 0) {
        if (readEventCallback_) {
            auto x =
                shared_from_this(); // readEventCallback可能会试图销毁这个对象
            // 在这里给引用计数加一以适当延长对象生命期
            // 从readEventCallback返回时可能对象内部的Chanel,Buffer对象已经失效
            readEventCallback_(x);
            if (!readBuf_)
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
            handleErroEvent();
        }
    }
}

void Connection::handleWritEvent() {
    if (writTasks_.empty()) {
        chan_->enableWrit(false);
        return;
    }
    auto ret = tryWrit(chan_->getChanelFd());
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
            handleErroEvent();
        }
    }
}

void Connection::handleErroEvent() {
    auto conn = shared_from_this();
    if (errnoEventCallback_) {
        errnoEventCallback_((conn));
    }
    destroy();
}

Connection &Connection::onRead(ConnCallback cb) {
    readEventCallback_ = cb;
    return *this;
}

Connection &Connection::onWrit(ConnCallback cb) {
    writeAllCallback_ = cb;
    return *this;
}

Connection &Connection::onErro(ConnCallback cb) {
    errnoEventCallback_ = cb;
    return *this;
}

Poller *Connection::getPoller() const { return poller_; }

void Connection::runEventHandler(BasicHandler handler) {
    assert(poller_);
    poller_->runEventHandler(handler);
}

void Connection::destroy() {
    verbose("try to destroy connection %p", this);
    chan_.reset();
    readBuf_.reset();
    writTasks_.clear();
    cancelAllTimer();
    writeAllCallback_ = nullptr;
    errnoEventCallback_ = nullptr;
    readEventCallback_ = nullptr;
    self_.reset();
}

int Connection::getConnfd() const { return chan_->filePtr()->fd(); }

void Connection::send(const char *buf, off_t len) {
    if (!readBuf_)
        return;
    io_task task;
    task.buffer_ = std::make_shared<Buffer>(buf, len);
    writTasks_.emplace_back(task);
    chan_->enableWrit();
    chan_->updateChanel();
    writBytes_ += len;
}

void Connection::send(Buffer &buf) {
    if (!readBuf_)
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
    if (!readBuf_)
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

void Connection::sendFile(std::shared_ptr<FileDescriptor> fp, off_t offset,
                          off_t len) {
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

Connection::~Connection() { verbose("DELETE CONNECTION %p", this); }

__thread char extraBuffer[163840];

ssize_t Connection::tryRead(int fd) {
    Buffer &buffer = *readBuf_;
    struct iovec iov[2];
    iov[0].iov_base = buffer.wdata();
    iov[0].iov_len = buffer.availbleSize();
    iov[1].iov_base = extraBuffer;
    iov[1].iov_len = sizeof(extraBuffer);
    int ret = ::readv(fd, iov, 2);
    if (ret > 0) {
        if (static_cast<decltype(iov[0].iov_len)>(ret) > iov[0].iov_len) {
            buffer.append(iov[0].iov_len);
            buffer.append(extraBuffer, ret - iov[0].iov_len);
        } else {
            buffer.append(ret);
        }
    }
    return ret;
}

ssize_t Connection::tryWrit(int fd) {
    ssize_t bytes = 0; // 注: sendfile的字节数不计入bytes中
    while (!writTasks_.empty()) {
        auto &first = writTasks_.front();
        if (first.fp_) {
#ifdef __linux__
            off_t offset = first.offset_;
            ssize_t ret = ::sendfile(fd, first.fp_->fd(), &offset, first.len_);
            if (ret < 0)
                return ret;
            if (ret == first.len_) {
                writTasks_.pop_front();
                continue;
            } else {
                first.len_ -= ret;
                first.offset_ += ret;
                return -1;
            }
#elif defined(__APPLE__)
            off_t len = first.len_;
            int ret = ::sendfile(first.fp_->fd(), fd, first.offset_, &len,
                                 nullptr, 0);
            if (ret < 0)
                return ret;
            if (len == first.len_) {
                writTasks_.pop_front();
                continue;
            } else {
                first.len_ -= len;
                first.offset_ += len;
                return -1;
            }
#endif
        } else {
            std::vector<struct iovec> iov;
            for (auto &x : writTasks_) {
                if (!x.fp_ && x.buffer_) {
                    struct iovec v;
                    v.iov_base = x.buffer_->data();
                    v.iov_len = x.buffer_->size();
                    iov.emplace_back(v);
                } else {
                    break;
                }
            }
            auto ret = ::writev(fd, &iov[0], iov.size());
            if (ret > 0) {
                bytes += ret;
                while (ret > 0) {
                    auto firstBufferSize = writTasks_.front().buffer_->size();
                    if (firstBufferSize <= ret) {
                        ret -= firstBufferSize;
                        writTasks_.pop_front();
                    } else {
                        writTasks_.front().buffer_->retrieve(ret);
                        return -1;
                    }
                }
            } else {
                return ret;
            }
        }
    }
    return bytes;
}
