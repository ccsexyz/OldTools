#ifndef LIBY_EPOLL_H
#define LIBY_EPOLL_H

#include "socket.h"
#include <sys/epoll.h>
#include <errno.h>

typedef struct epoller_ epoller_t;
typedef void (*epoll_event_handler)(epoller_t *, int);
typedef void (*epoll_hook)(epoller_t *);

typedef struct epoller_ {
    int epfd;
    int epollsize;
    int flag;
    int error_code;
    int running_servers;

    epoll_event_handler event_handler;
    epoll_hook error_hook;
    epoll_hook destroy_hook;

    struct epoll_event *events;
    struct epoll_event event;
} epoller_t;

enum { CTL_ERROR };

void *safe_malloc(size_t n);
void run_epoll_main_loop(epoller_t *loop, epoll_event_handler handler);

epoller_t *epoller_init(int epollsize);
void epoller_destroy(epoller_t *loop);
static void ctl(epoller_t *loop, int fd, int op);
void epoll_add(epoller_t *loop, int fd);
void epoll_mod(epoller_t *loop, int fd);
void epoll_del(epoller_t *loop, int fd);


#endif
