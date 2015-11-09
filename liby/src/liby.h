#include "base.h"
#include "epoll.h"
#include "socket.h"

typedef void (*accept_func)(void);
typedef void (*hook_func)(void *);
typedef hook_func handle_func;

typedef struct io_task_ {
    char *buf;
    off_t size;
    off_t offset;
} io_task;

typedef struct {
    int fd;

    char *wbuf;
    off_t woffset;
    off_t wsend;
    off_t wsize;

    char *rbuf;
    off_t roffset;
    off_t rread;
    off_t rsize;
} socket_t;

typedef struct liby_server_ {
    int type;
    char *server_name;
    char *server_port;
    int listenfd;

    off_t wsize;
    off_t rsize;

    epoller_t *loop;

    handle_func acceptor;

    handle_func read_complete_handler;
    handle_func write_complete_handler;
    handle_func error_handler;

    liby_client *head, *tail;
} liby_server;

typedef struct liby_client_ {
    int type;

    io_task *read_list_head, *read_list_tail;
    io_task *write_list_head, *write_list_tail;
    mutex_t io_read_mutex;
    mutex_t io_write_mutex;

    socket_t *sock;
    liby_server *server;
    struct liby_client_ *next, *prev;
} liby_client;

liby_server *liby_server_init(const char *server_name, const char *server_port);
void liby_server_destroy(liby_server *server);
void add_server_to_epoller(liby_server *server, epoller_t *loop);


liby_server *
liby_server_init(const char *server_name, const char *server_port)
{
    if(server_name == NULL || server_port == NULL) {
        fprintf(stderr, "server name or port must be valid!\n");
        return NULL;
    }

    char *name = strdup(server_name); //must be free
    char *port = strdup(server_port);

    liby_server *server = safe_malloc(sizeof(liby_server));
    memset((void *)server, 0, sizeof(liby_server));

    server->server_name = name;
    server->server_port = port;
    server->listenfd = initserver(name, port);

    return server;
}

void
liby_server_destroy(liby_server *server)
{
    if(server) {
        if(server->server_name) free(server->server_name);
        if(server->server_port) free(server->server_port);
        close(listenfd);

        free(server);
    }
}

void
add_server_to_epoller(liby_server *server, epoller_t *loop)
{
    if(server == NULL || loop == NULL) {
        fprintf(stderr, "server or epoller must be invalid!\n");
        return;
    }

    server->loop = loop;

    struct epoll_event *event = &(loop->event);
    event->data.ptr = (void *)server;
    event->events = EPOLLIN | EPOLLHUP;

    if(loop->running_servers == 0)
        loop->flag = !0;
    loop->running_servers++;
}

void
epoll_event_handler(epoller_t *loop, int n)
{
    for(int i = 0; loop->flag && (i < n); i++) {
        struct epoll_event *p = &(loop->events[i]);

        liby_server *server = (liby_server *)(p->data.ptr);

        if(server->type) {
            epoll_acceptor(server);
            continue;
        }

        liby_client *client = (liby_client *)(p->data.ptr);
        if(p->events & EPOLLHUP) {
            del_client_from_server(client, client->server);
            continue;
        } else if(p->events & EPOLLIN) {
            read_message(client);
        } else if(p->events & EPOLLOUT) {
            write_message(client);
        }
    }
}

void
epoll_acceptor(liby_server *server)
{
    if(s == NULL) return;
    int listen_fd = s->listen_fd;

    while(1) {
        int clfd = accept(listen_fd, NULL, NULL);
        if(clfd < 0) {
            if(errno == EAGAIN) {
                break;
            } else {
                fprintf(stderr, "accept error: %s\n", strerror(errno));
                break;
            }
        }

        set_noblock(clfd);

        liby_client *client = liby_client_init(clfd, server);
        add_client_to_server(client, server);

        if(server->acceptor)
            server->acceptor((void *)client);
    }
}

socket_t *
listen_socket_init(int fd)
{
    return socket_init(fd, 0, 0);
}

socket_t *
socket_init(int fd, off_t wsize, off_t rsize)
{
    socket_t *s = (socket_t *)safe_malloc(sizeof(socket_t));
    memset((void *)s, 0, sizeof(socket_t));
    s->fd = fd;

    if(wsize != 0) {
        s->wsize = wsize;
        s->rbuf = (char *)safe_malloc(wsize);
    }

    if(rsize != 0) {
        s->rsize = rsize;
        s->rbuf = (char *)safe_malloc(rsize);
    }
}

void
socket_release(socket_t *s)
{
    if(s == NULL) return;

    close(s->fd);

    if(s->wbuf) free(s->wbuf);
    if(s->rbuf) free(s->rbuf);

    free(s);
}

liby_client *
liby_client_init(int fd, liby_server *server)
{
    if(s == NULL) {
        //
        close(fd);
        return NULL;
    }

    liby_client *c = (liby_client *)safe_malloc(sizeof(liby_client));
    memset((void *)c, 0, sizeof(liby_client));
    //c->type = 0;
    c->sock = socket_init(fd, server->wsize, server->rsize);
    c->server = server;

    return c;
}

void
liby_client_release(liby_client *c)
{
    if(c == NULL) {
        return;
    }

    socket_release(c->sock);
    free(c);
}

void
add_client_to_server(liby_client *client, liby_server *server)
{
    if(client == NULL || server == NULL) {
        return;
    }

    liby_client *p = server->tail;
    if(p == NULL) {
        server->head = server->tail = client;
        client->next = NULL;
        client->prev = NULL;
    } else {
        server->tail->next = client;
        client->prev = server->tail;
        server->tail = client;
        client->next = NULL;
    }
}

void
del_client_from_server(liby_client *client, liby_server *server)
{
    if(client == NULL || server == NULL) {
        return;
    }

    liby_client *p = server->head;
    while(p) {
        if(p == client) {
            if(p->prev != NULL) {
                p->prev->next = p->next;
            }
            if(p->next != NULL) {
                p->next->prev = p->prev;
            }
            if(p == server->head) {
                server->head = p->next;
            }
            if(p == server->tail) {
                server->tail = p->prev;
            }

            liby_client_release(client);

            return;
        } else {
            p = p->next;
        }
    }

    fprintf(stderr, "no this client with pointer %p\n", client);
}

void
read_message(liby_client *client)
{
    if(client == NULL) return;

    liby_server *server = client->server;
    if(server == NULL) {
        liby_client_release(client);
        return;
    }

    socket_t *sock = client->sock;
    int clfd = sock->fd;
    epoller_t *loop = server->loop;
    struct epoll_event *event = &(loop->event);
    char *rbuf = sock->rbuf;
    off_t *rread = &(sock->rread);
    off_t *roffset = &(sock->roffset);

    while(1) {
        int ret = read(clfd, rbuf + *roffset, *rread - *roffset);
        if(ret > 0) {
            *roffset += ret;
            if(*roffset >= *rread) { //read complete
                ;
                *woffset = *wsend = 0;
                if(server->read_complete_handler)
                    server->read_complete_handler((void *)client);
                break;
            }
        } else {
            if(errno == EAGAIN) { //read not complete
                ;
            } else {
                if(server->error_handler) server->error_handler((void *)client);
                del_client_from_server(client, server);
            }
            break;
        }
    }
}

void
write_message(liby_client *client)
{
    if(client == NULL) return;

    liby_server *server = client->server;
    if(server == NULL) {
        liby_client_release(client);
        return;
    }

    socket_t *sock = client->sock;
    int clfd = sock->fd;
    epoller_t *loop = server->loop;
    struct epoll_event *event = &(loop->event);
    char *wbuf = sock->wbuf;
    off_t *wsend = &(sock->wsend);
    off_t *woffset = &(sock->woffset);

    while(1) {
        int ret = write(clfd, wbuf + *woffset, *wsend - *woffset);
        if(ret > 0) {
            *woffset += ret;
            if(*woffset == *wsend) { //write complete
                ;
                *woffset = *wsend = 0;
                if(server->write_complete_handler)
                    server->write_complete_handler((void *)client);

                break;
            } else {
                if(errno == EAGAIN) { //write not complete
                    ;
                } else {
                    if(server->error_handler) server->error_handler((void *)client);
                    del_client_from_server(client, server);
                }

                break;
            }
        }
    }
}

void
push_io_task_to_client(io_task *task, liby_client *client, int type) //type == true read type == false write
{
    if(task == NULL || client == NULL) {
        return;
    }

    mutex_t *m =
}
