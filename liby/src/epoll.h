#ifndef LIBY_EPOLL_H
#define LIBY_EPOLL_H

#include "socket.h"
#include <errno.h>
#include <sys/epoll.h>

typedef struct epoller_ epoller_t;
typedef struct chan_ chan;
typedef void (*epoll_event_handler)(chan *c);

typedef struct chan_ {
    void *p;
    int fd;
    struct epoll_event *event;
    epoll_event_handler event_handler;
} chan;

typedef struct epoller_ {
    int epfd;
    int epollsize;
    int flag;
    int error_code;

    struct epoll_event *events;
} epoller_t;

enum { CTL_ERROR };

void run_epoll_main_loop(epoller_t *epoller);

epoller_t *epoller_init(int epollsize);

void epoller_destroy(epoller_t *epoller);

void add_chan_to_epoller(epoller_t *epoller, chan *ch);

void remove_chan_from_epoller(epoller_t *epoller, chan *ch);

void mod_chan_of_epoller(epoller_t *epoller, chan *ch);

chan *make_chan(void *p, int fd, struct epoll_event *event, epoll_event_handler event_handler);

void destroy_chan(chan *ch);

#endif
