#include "epoll.h"
#include "log.h"
#include "base.h"

static int i = 1;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void ctl(epoller_t *epoller, int fd, int op, struct epoll_event *event) {
    assert(epoller && fd >= 0);

    if (epoll_ctl(epoller->epfd, op, fd, event) < 0) {
        log_err("epoll_ctl error: %s with fd %d", strerror(errno), fd);
    }
}

static void epoll_mod(epoller_t *epoller, int fd, struct epoll_event *event) {
    ctl(epoller, fd, EPOLL_CTL_MOD, event);
}

static void epoll_add(epoller_t *epoller, int fd, struct epoll_event *event) {
    ctl(epoller, fd, EPOLL_CTL_ADD, event);
}

static void epoll_del(epoller_t *epoller, int fd, struct epoll_event *event) {
    ctl(epoller, fd, EPOLL_CTL_DEL, event);
}

void run_epoll_main_loop(epoller_t *epoller) {
    epoller->flag = 1;

    for (int nfds; epoller->flag;) {
        nfds = epoll_wait(epoller->epfd, epoller->events, epoller->epollsize, -1);
        log_info("event from thread %llx!", pthread_self());
        if (nfds == 0)
            continue;
        if(nfds < 0 && errno != EINTR)
            log_quit("epoll_wait: %s", strerror(errno));

        for(int i = 0; i < nfds; i++) {
            chan *ch = (chan *)epoller->events[i].data.ptr;
            assert(ch);
            //*(ch->event) = epoller->events[i];
            ch->catched_event = &(epoller->events[i]);
            ch->event_handler(ch);
        }
    }
}

epoller_t *epoller_init(int epollsize) {
    if (epollsize <= 0)
        epollsize = 10240;
    int epfd = epoll_create(epollsize);
    if (epfd < 0)
        return NULL;

    epoller_t *epoller = safe_malloc(sizeof(epoller_t));
    memset((void *)epoller, 0, sizeof(epoller_t));
    epoller->events = safe_malloc(sizeof(struct epoll_event) * epollsize);
    memset((void *)(epoller->events), 0, sizeof(struct epoll_event) * epollsize);
    epoller->epfd = epfd;
    epoller->epollsize = epollsize;

    return epoller;
}

void epoller_destroy(epoller_t *epoller) {
    assert(epoller && epoller->epfd >= 0 && epoller->events);
    close(epoller->epfd);
    free(epoller->events);
    free(epoller);
}

void add_chan_to_epoller(epoller_t *epoller, chan *ch) {
    assert(epoller && ch && ch->fd >= 0 && ch->p && ch->event && ch->event_handler);
    //pthread_mutex_lock(&mutex);
    epoll_add(epoller, ch->fd, ch->event);
    //pthread_mutex_unlock(&mutex);
}

void remove_chan_from_epoller(epoller_t *epoller, chan *ch) {
    assert(epoller && ch && ch->fd >= 0);
    epoll_del(epoller, ch->fd, ch->event);
    //pthread_mutex_lock(&mutex);
    //pthread_mutex_unlock(&mutex);
}

void mod_chan_of_epoller(epoller_t *epoller, chan *ch) {
    assert(epoller && ch && ch->fd >= 0 && ch->event);
    //pthread_mutex_lock(&mutex);
    epoll_mod(epoller, ch->fd, ch->event);
    //pthread_mutex_unlock(&mutex);
}

chan *make_chan(void *p, int fd, struct epoll_event *event, epoll_event_handler event_handler) {
    ALLOC(chan, ret);
    ret->p = p;
    ret->fd = fd;
    ret->event = event;
    ret->catched_event = NULL;
    ret->event_handler = event_handler;
    return ret;
}

void destroy_chan(chan *ch) {
    assert(ch && ch->fd >= 0);
    int ret = close(ch->fd);
    if(ret < 0) log_err("close");
    free(ch);
}
