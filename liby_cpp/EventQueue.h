#ifndef POLLERTEST_EVENTQUEUE_H
#define POLLERTEST_EVENTQUEUE_H

#include "BlockingQueue.h"
#include "util.h"

namespace Liby {
class File;
class Chanel;
class Poller;

class EventQueue : clean_ {
public:
    EventQueue(Poller *poller);
    void setPoller(Poller *poller);
    void start();
    void destroy();
    void wakeup();
    void pushHandler(const BasicHandler &handler);

private:
    Poller *poller_;
    int eventfd_ = -1;
    std::shared_ptr<File> eventfp_;
#ifdef __APPLE__
    int event2fd_ = -1;
    std::shared_ptr<File> event2fp_;
#endif
    std::unique_ptr<Chanel> eventChanelPtr_;
    std::unique_ptr<BlockingQueue<BasicHandler>>
        eventHandlersPtr_; // BlockingQueue is nocopyable, so use smart-pointer
};
}

#endif // POLLERTEST_EVENTQUEUE_H
