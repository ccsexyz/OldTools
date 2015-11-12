#include "base.h"
#include "epoll.h"
#include "socket.h"

#define DEFAULT_BUFFER_SIZE (4096)

typedef void (*accept_func)(liby_client *);
typedef void (*hook_func)(void *);
typedef void (*handle_func)(liby_client *, char *, off_t, int);

typedef struct io_task_ {
    char *buf;
    off_t size;
    off_t offset;
    off_t min_except_bytes;
    handle_func handler;   //be set to NULL if it does not need any handler
} io_task;

typedef struct liby_server_ {
    int type;
    char *server_path;
    char *server_port;
    int listenfd;

    epoller_t *loop;

    handle_func acceptor;

    handle_func read_complete_handler;
    handle_func write_complete_handler;
    handle_func close_handler;
    handle_func error_handler;

    liby_client *head, *tail;
} liby_server;

typedef struct liby_client_ {
    int type;
    int sockfd;

    io_task *read_list_head, *read_list_tail, *curr_read_task;
    io_task *write_list_head, *write_list_tail, *curr_write_task;
    mutex_t io_read_mutex;
    mutex_t io_write_mutex;

    liby_server *server;
    struct liby_client_ *next, *prev;
} liby_client;

liby_server *liby_server_init(const char *server_name, const char *server_port);
void liby_server_destroy(liby_server *server);
void add_server_to_epoller(liby_server *server, epoller_t *loop);


liby_server *
liby_server_init(const char *server_path, const char *server_port)
{
    if(server_name == NULL || server_port == NULL) {
        fprintf(stderr, "server name or port must be valid!\n");
        return NULL;
    }

    char *path = strdup(server_path); //must be free
    char *port = strdup(server_port);

    liby_server *server = safe_malloc(sizeof(liby_server));
    memset((void *)server, 0, sizeof(liby_server));

    server->server_path = path;
    server->server_port = port;
    server->listenfd = initserver(path, port);

    return server;
}

void
liby_server_destroy(liby_server *server)
{
    if(server) {
        if(server->server_path) free(server->server_path);
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

    if(client->curr_read_task == NULL) {
        client->curr_read_task = pop_io_task_from_client(client, true);
        if(client->curr_read_task == NULL) {
            return;
        }
    }

    while(client->curr_read_task) {
        io_task *p = client->curr_read_task;

        while(1) {
            int ret = read(client->sockfd, p->buf + p->offset, p->size - p->offset);
            if(ret > 0) {
                p->offset += ret;
                if(p->offset >= p->min_except_bytes) {
                    if(p->handler) {
                        p->handler(client, p->buf, p->offset, 0);
                    }
                    if(server->read_complete_handler) {
                        server->read_complete_handler(client, p->buf, p->offset, 0);
                    }
                    break;
                }
            } else {
                if(errno == EAGAIN) {
                    return;
                } else {
                    if(p->handler) {
                        p->handler(client, p->buf, p->offset, errno);
                    }
                    if(server->read_complete_handler) {
                        server->read_complete_handler(client, p->buf, p->offset, errno);
                    }

                    del_client_from_server(client, server);

                    return;
                }
            }
        }

        client->curr_read_task = pop_io_task_from_client(client, true);
        free(p);
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

    if(client->curr_write_task == NULL) {
        client->curr_write_task = pop_io_task_from_client(client, false);
        if(client->curr_write_task == NULL) {
            return;
        }
    }

    while(client->curr_write_task) {
        io_task *p = client->curr_write_task;

        while(1) {
            int ret = write(client->sockfd, p->buf + p->offset, p->size - p->offset);
            if(ret > 0) {
                p->offset += ret;
                if(p->offset >= p->min_except_bytes) {
                    if(p->handler) {
                        p->handler(client, p->buf, p->offset, 0);
                    }
                    if(server->write_complete_handler) {
                        server->write_complete_handler(client, p->buf, p->offset, 0);
                    }
                    break;
                }
            } else {
                if(errno == EAGAIN) {
                    return;
                } else {
                    if(p->handler) {
                        p->handler(p->buf, p->offset, errno);
                    }
                    if(server->write_complete_handler) {
                        server->write_complete_handler(p->buf, p->offset, errno);
                    }

                    del_client_from_server(client, server);

                    return;
                }
            }
        }

        client->curr_write_task = pop_io_task_from_client(client, false);
        free(p);
    }
}

void
push_io_task_to_client(io_task *task, liby_client *client, int type) //type == true read type == false write
{
    if(task == NULL || client == NULL) {
        return;
    }

    if(type) {
        LOCK(client->io_read_mutex);
        push_io_task(&(client->read_list_head), &(client->read_list_tail), task);
        UNLOCK(client->io_read_mutex);
    } else {
        LOCK(client->io_write_mutex);
        push_io_task(&(client->write_list_head), &(client->write_list_tail), task);
        UNLOCK(client->io_write_mutex);
    }
}

void
push_io_task(io_task **head, io_task **tail, io_task *p)
{
    if(*head == NULL) {
        *head = *tail = p;
        p->next = p->prev = NULL;
    } else {
        p->prev = *tail;
        (*tail)->next = p;
        (*tail) = p;
    }
}

io_task *
pop_io_task(io_task **head, io_task **tail)
{
    if(*head == NULL) {
        return NULL;
    } else {
        io_task *p = *head;
        *head = p->next;

        if(*head == NULL) {
            *tail = NULL;
        }

        return p;
    }
}

io_task *
pop_io_task_from_client(liby_client *client, int type) // true for read, false for write
{
    io_task *ret = NULL;

    if(task == NULL || client == NULL) {
        return NULL;
    }

    if(type) {
        LOCK(client->io_read_mutex);
        ret = pop_io_task(&(client->read_list_head), &(client->read_list_tail));
        UNLOCK(client->io_read_mutex);
    } else {
        LOCK(client->io_write_mutex);
        ret = pop_io_task(&(client->write_list_head), &(client->write_list_tail));
        UNLOCK(client->io_write_mutex);
    }

    return ret;
}

void
liby_async_read_some(liby_client *client, char *buf, off_t buffersize, handle_func handler)
{
    if(client == NULL) return;

    if(buf == NULL) {
        buf = safe_malloc(DEFAULT_BUFFER_SIZE);
        buffersize = DEFAULT_BUFFER_SIZE;
    }

    io_task *task = (io_task *)safe_malloc(sizeof(io_task));
    task->buf = buf;
    task->size = buffersize;
    task->offset = 0;
    task->min_except_bytes = 0;
    task->handler = handler;

    push_io_task_to_client(task, client, true);
}

void
liby_async_read(liby_client *client, handle_func handler)
{
    liby_async_write_some(client, NULL, 0, handler);
}

void
liby_async_write_some(liby_client *client, char *buf, off_t buffersize, handle_func handler)
{
    if(client == NULL) return;
    if(buf == NULL || buffersize == 0) return;

    io_task *task = (io_task *)safe_malloc(sizeof(io_task));
    task->buf = buf;
    task->size = buffersize;
    task->offset = 0;
    task->min_except_bytes = 0;
    task->handler = handler;

    push_io_task_to_client(task, client, true);
}

void
set_write_complete_handler_for_server(handle_func handler, liby_server *server)
{
    if(server != NULL) {
        server->write_complete_handler = handler;
    }
}

void
set_read_complete_handler_for_server(handle_func handler, liby_server *server)
{
    if(server != NULL) {
        server->read_complete_handler = handler;
    }
}

void
set_acceptor_for_server(accept_func acceptor, liby_server *server)
{
    if(server != NULL) {
        server->acceptor = acceptor;
    }
}
