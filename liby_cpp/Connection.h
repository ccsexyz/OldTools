#ifndef POLLERTEST_CONNECTION_H
#define POLLERTEST_CONNECTION_H

#include "Buffer.h"
#include "Liby.h"
#include "util.h"
#include <memory>
#include <unordered_set>

namespace Liby {
class Chanel;
class File;
class Poller;
class File;

class Connection final : clean_,
                         public TimerSet,
                         public std::enable_shared_from_this<Connection> {
public:
    using TimerId = uint64_t;
    const size_t defaultWritableLimit = 16 * 4096;
    using ConnCallback = std::function<void(std::shared_ptr<Connection>)>;
    Connection(Poller *poller, std::shared_ptr<File> &fp);
    ~Connection();

    void init();

    void send(void *base, off_t len) { send(static_cast<const char *>(base), len); }
    void send(const char *buf, off_t len);
    void send(Buffer &buf);
    void send(Buffer &&buf);
    void send(const char *buf);
    void send(const char c);
    Buffer &read();

    void setReadCallback(ConnCallback cb);
    void setWritCallback(ConnCallback cb);
    void setErroCallback(ConnCallback cb);
    void setWritLimitCallback(ConnCallback cb);

    void setWritableLimit(size_t writableLimit);

    Poller *getPoller() const;
    int getConnfd() const;
    void sendFile(std::shared_ptr<File> fp, off_t offset, off_t len);
    void runEventHandler(BasicHandler handler);
//    void cancelAllTimer();
//    void cancelTimer(TimerId id);
//    TimerId runAt(const Timestamp &timestamp, const BasicHandler &handler);
//    TimerId runAfter(const Timestamp &timestamp, const BasicHandler &handler);
//    TimerId runEvery(const Timestamp &timestamp, const BasicHandler &handler);

    void suspendRead(bool flag = true);
    void suspendWrit(bool flag = true);

    void destroy();
    void setErro();

private:
    void onRead();
    void onWrit();
    void onErro();

public:
    off_t writableLimit_ = defaultWritableLimit;
    void *udata_ = nullptr;

private:
    off_t writBytes_ = 0;
    Poller *poller_ = nullptr;
    std::unordered_set<TimerId> timerIds_;
    std::unique_ptr<Chanel> chan_;
    std::unique_ptr<Buffer> readBuf_;
//    std::unique_ptr<Buffer> writBuf_;
    std::deque<io_task> writTasks_;
    ConnCallback readEventCallback_;
    ConnCallback writeAllCallback_;
    ConnCallback errnoEventCallback_;
    ConnCallback writableLimitCallback_;
};
}

#endif // POLLERTEST_CONNECTION_H
