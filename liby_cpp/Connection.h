#ifndef LIBY_CONNECTION_H
#define LIBY_CONNECTION_H

#include "BlockingQueue.h"
#include "Chanel.h"
#include "Poller.h"
#include "util.h"
#include <condition_variable>
#include <mutex>

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

using io_handler = std::function<void(char *, off_t, int)>;

class io_task final {
public:
    ~io_task() {
        if (shouldDelete_)
            delete[] buf_;
    }
    void setAutoDelete() { shouldDelete_ = true; }
    bool shouldDelete_ = false;
    char *buf_ = nullptr;
    off_t length_ = 0;
    off_t offset_ = 0;
    off_t min_except_bytes = 0;
    io_handler handler_;
};

//  : public std::enable_shared_from_this<Connection>
class Connection final {
public:
    using EventCallback = std::function<void(ConnectionPtr &)>;
    Connection(ChanelPtr &&chan) : chan_(std::move(chan)) { init(); }
    Connection(const ChanelPtr &chan) : chan_(chan) { init(); }

    void async_read(char *buf, off_t buffersize, io_handler handler) {
        async_read_some(buf, buffersize, 0, handler);
    }
    void async_read(io_handler handler) {
        async_read_some(new char[defaultBufferSize_], defaultBufferSize_, 0,
                        handler);
    }
    void async_read_some(char *buf, off_t buffersize, off_t min_except_bytes,
                         io_handler handler);

    void async_writ(char *buf, off_t buffersize, io_handler handler) {
        async_writ_some(buf, buffersize, buffersize, handler);
    }
    void async_writ_some(char *buf, off_t buffersize, off_t min_except_bytes,
                         io_handler handler);

    void setDefaultBufferSize(size_t newSize) { defaultBufferSize_ = newSize; }
    void updateConnectionState();
    void setPoller(Poller *poller);

private:
    void init();
    void handleReadEvent();
    void handleWritEvent();
    void handleErroEvent();

private:
    int defaultBufferSize_ = 4096;
    ChanelPtr chan_;
    BlockingQueue<io_task> readTaskQueue_;
    BlockingQueue<io_task> writTaskQueue_;
};

#endif // LIBY_CONNECTION_H
