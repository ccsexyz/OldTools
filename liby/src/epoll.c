#include "epoll.h"

void *
safe_malloc(size_t n)
{
    if(n < 0)
        goto err;
    void *p = malloc(n);
    if(p != NULL)
        return p;
err:
    puts("malloc error!\n");
    exit(1);
}


void
run_epoll_main_loop(epoller_t *loop)
{
    for(int nfds; loop->flag; ) {
        nfds = epoll_wait(loop->epfd, loop->events, loop->epollsize, -1);
        if(nfds <= 0) continue;

        if(loop->event_handler) loop->event_handler(loop, nfds);
    }
}

epoller_t *
epoller_init(int epollsize)
{
    if(epollsize <= 0) epollsize = 10240;
    int epfd = epoll_create(epollsize);
    if(epfd < 0) return NULL;

    epoller_t *loop = safe_malloc(sizeof(epoller_t));
    memset((void *)loop, 0, sizeof(epoller_t));
    loop->events = safe_malloc(sizeof(struct epoll_event) * epollsize);
    memset((void *)(loop->events), 0, sizeof(struct epoll_event));
    loop->epfd = epfd;
    loop->epollsize = epollsize;

    return loop;
}

void
epoller_destroy(epoller_t *loop)
{
    if(loop == NULL) return;

    if(loop->destroy_hook) destroy_hook(loop);

    close(loop->epfd);
    if(events) free(events);
}

static void
ctl(epoller_t *loop, int fd, int op)
{
    if(loop == NULL) return;

    if(epoll_ctl(loop->epfd, op, fd, &(loop->event)) < 0) {
        fprintf(stderr, "epoll_ctl error: %s\n", strerror(errno));
        if(loop->error_hook) {
            int temp = loop->error_code;
            loop->error_code = CTL_ERROR;
            loop->error_hook(loop);
            loop->error_code = temp;
        }
    }
}

void
epoll_add(epoller_t *loop, int fd)
{
    ctl(loop, fd, EPOLL_CTL_ADD);
}

void
epoll_mod(epoller_t *loop, int fd)
{
    ctl(loop, fd, EPOLL_CTL_MOD);
}

void
epoll_del(epoller_t *loop, int fd)
{
    ctl(loop, fd, EPOLL_CTL_DEL);
}
