#include "Chanel.h"
#include "Poller.h"

using namespace Liby;

void Chanel::handleEvent() {
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

void Chanel::removeChanel() {
    assert(poller_);
    poller_->removeChanel(this);
}

void Chanel::updateChanel() {
    assert(poller_);
    poller_->updateChanel(this);
}

void Chanel::addChanel() {
    assert(poller_);
    poller_->addChanel(this);
}
