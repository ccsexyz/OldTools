#include "Channel.h"
#include "Poller.h"

using namespace Liby;

Channel::~Channel() { removeChanel(); }

void Channel::handleEvent() {
    savedEvents_ = events_;
    if (isError()) {
        if (erroEventCallback_) {
            erroEventCallback_();
        } else {
            error("Error in fd %d but no erroEventCallback", fp_->fd());
        }
    } else if (Readable()) {
        if (readEventCallback_) {
            readEventCallback_();
        } else {
            error("Read event int fd %d without readEventCallback", fp_->fd());
        }
    } else if (Writable()) {
        if (writEventCallback_) {
            writEventCallback_();
        } else {
            error("Writ event in fd %d without writEventCallback", fp_->fd());
        }
    }
}

void Channel::removeChanel() {
    // 这里不对poller_作断言
    //　这是为了解决Poller中的TimerQueue和EventQueue在析构时会间接调用纯虚函数的问题
    //　如果不这样解决，我能想到的办法就只有将这两个类移出Poller虚基类,这样会导致一定的代码膨胀的问题
    if (poller_)
        poller_->removeChanel(this);
}

void Channel::updateChanel() {
    assert(poller_);
    poller_->updateChanel(this);
    savedEvents_ = events_;
}

void Channel::addChanel() {
    assert(poller_);
    poller_->addChanel(this);
    savedEvents_ = events_;
}
