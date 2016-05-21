#ifndef POLLERTEST_EVENTQUEUE_H
#define POLLERTEST_EVENTQUEUE_H

#include "BlockingQueue.h"
#include "util.h"

namespace Liby {
class EventQueue : clean_ {
public:
    EventQueue(Poller *poller);
    ~EventQueue();
    void setPoller(Poller *poller);
    void start();
    void destroy();
    void wakeup();
    void pushHandler(const BasicHandler &handler);

private:
    Poller *poller_;
    int eventfd_ = -1;
    std::shared_ptr<FileDescriptor> eventfp_;
#ifdef __APPLE__
    int event2fd_ = -1;
    std::shared_ptr<FileDescriptor> event2fp_;
#endif
    std::unique_ptr<Channel> eventChanelPtr_;
    std::unique_ptr<BlockingQueue<BasicHandler>>
        eventHandlersPtr_; // BlockingQueue is nocopyable, so use smart-pointer
};
}

#endif // POLLERTEST_EVENTQUEUE_H
