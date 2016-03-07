#include "liby.h"
#include "log.h"

static void *busy_worker(void *p);
static void io_task_destroy(io_task *p);
static void liby_server_acceptor(chan *c);
static void liby_client_handler(chan *c);
static void read_message(liby_client *client);
static void write_message(liby_client *client);
static void push_read_task_to_client(io_task *task, liby_client *client);
static io_task *pop_read_task_from_client(liby_client *client);
static void push_write_task_to_client(io_task *task, liby_client *client);
static io_task *pop_write_task_from_client(liby_client *client);

static void *busy_worker(void *p) {
    assert(p);
    run_epoll_main_loop((epoller_t *)p);
    pthread_exit(NULL);
}

liby_loop *liby_create_easy(void) {
    return liby_loop_create(get_hardware_concurrency());
}

liby_loop *liby_loop_create(int nthreads) {
    ALLOC(liby_loop, loop);
    BZERO(loop);
    loop->maxfds = max_fds();
    loop->nthreads = nthreads;
    if (nthreads > 0) {
        NALLOC(pthread_t, temp, nthreads);
        loop->tids = temp;
    }
    {
        NALLOC(epoller_t *, temp, (nthreads + 1));
        loop->epollers = temp;
    }
    for (int i = 0; i < (nthreads + 1); i++) {
        loop->epollers[i] = epoller_init(loop->maxfds);
    }
    for (int i = 0; i < nthreads; i++) {
        int err = pthread_create(loop->tids + i, NULL, busy_worker,
                                 (void *)(loop->epollers[i + 1]));
        if (err < 0)
            log_quit("create thread error!");
    }
    INIT_LIST_HEAD(&(loop->clients.list));
    INIT_LIST_HEAD(&(loop->servers.list));
    return loop;
}

void run_liby_main_loop(liby_loop *loop) {
    assert(loop && loop->epollers);
    run_epoll_main_loop(loop->epollers[0]);
    liby_loop_destroy(loop);
}

void liby_loop_destroy(liby_loop *loop) {
    if (loop != NULL) {
        loop->keep_running = 0;

        for (int i = 0; i < loop->nthreads; i++) {
            pthread_join(loop->tids[i], NULL);
        }
        for (int i = 0; i < loop->nthreads + 1; i++) {
            epoller_destroy(loop->epollers[i]);
        }

        struct list_head *pos = NULL;
        liby_client *pnode = NULL;
        list_for_each(pos, &(loop->clients.list)) {
            pnode = list_entry(pos, liby_client, list);
            liby_client_destroy(pnode);
        }

        pos = NULL;
        liby_server *snode = NULL;
        list_for_each(pos, &(loop->servers.list)) {
            snode = list_entry(pos, liby_server, list);
            liby_server_destroy(snode);
        }
    }
}

liby_client *liby_client_init(int clfd, liby_loop *loop) {
    assert(loop && clfd >= 0);

    set_noblock(clfd);

    ALLOC(liby_client, client);
    BZERO(client);

    struct epoll_event *event = &(client->event);
    //event->data.fd = clfd;
    event->events = EPOLLIN | EPOLLHUP | EPOLLET;
    client->ch = make_chan((void *)client, clfd, event, liby_client_handler);
    event->data.ptr = (void *)(client->ch);

    client->sockfd = clfd;
    client->loop = loop;
    mutex_init(&(client->io_read_mutex));
    mutex_init(&(client->io_write_mutex));
    INIT_LIST_HEAD(&(client->read_list_head.list));
    INIT_LIST_HEAD(&(client->write_list_head.list));

    if (loop->nthreads == 0) {
        client->epoller = loop->epollers[0];
    } else {
        client->epoller = loop->epollers[1 + clfd % loop->nthreads];
    }

    add_chan_to_epoller(client->epoller, client->ch);

    return client;
}


void liby_client_destroy(liby_client *client) {
    assert(client);

    if (client->server) {
        remove_client_from_server(client);
    } else if(client->loop){
        remove_client_from_loop(client);
    }

    if(client->curr_read_task) io_task_destroy(client->curr_read_task);
    if(client->curr_write_task) io_task_destroy(client->curr_write_task);

    if(!list_empty(&(client->read_list_head.list))) {
        struct list_head *pos = NULL;
        io_task *task = NULL;
        list_for_each(pos, &(client->read_list_head.list)) {
            task = list_entry(pos, io_task, list);
            if(task && task->handler) {
                task->handler(client, task->buf, 0, -1);
                io_task_destroy(task);
            }
        }
    }

    if(!list_empty(&(client->write_list_head.list))) {
        struct list_head *pos = NULL;
        io_task *task = NULL;
        list_for_each(pos, &(client->write_list_head.list)) {
            task = list_entry(pos, io_task, list);
            if(task && task->handler) {
                task->handler(client, task->buf, 0, -1);
                io_task_destroy(task);
            }
        }
    }

    mutex_destroy(&(client->io_read_mutex));
    mutex_destroy(&(client->io_write_mutex));

    if (client->release_data_func) {
        client->release_data_func(client->data);
    } else {
        free(client->data);
    }

    destroy_chan(client->ch);
    free(client);
}

liby_server *liby_server_init(const char *server_path, const char *server_port,
                              accept_func acceptor) {
    assert(server_path && server_port && acceptor);

    char *path = strdup(server_path);
    char *port = strdup(server_port);

    ALLOC(liby_server, server);
    BZERO(server);

    server->server_path = path;
    server->server_port = port;
    server->acceptor = acceptor;
    server->listenfd = initserver(path, port);
    set_noblock(server->listenfd);
    INIT_LIST_HEAD(&(server->clients.list));
    mutex_init(&(server->clients_mutex));
    server->ch = make_chan((void *)server, server->listenfd, &(server->event), liby_server_acceptor);

    return server;
}

void liby_server_destroy(liby_server *server) {
    assert(server && server->ch && server->listenfd >= 0);

    if (server->server_path)
        free(server->server_path);
    if (server->server_port)
        free(server->server_port);
    struct list_head *pos = NULL;
    liby_client *pnode = NULL;
    list_for_each(pos, &(server->clients.list)) {
        pnode = list_entry(pos, liby_client, list);
        liby_client_destroy(pnode);
    }

    destroy_chan(server->ch);
    free(server);
}

void add_server_to_loop(liby_server *server, liby_loop *loop) {
    assert(server && loop);
    server->loop = loop;
    server->epoller = loop->epollers[0];

    list_add_tail(&(server->list), &(loop->servers.list));

    struct epoll_event *event = &(server->event);
    event->data.ptr = server->ch;
    event->events = EPOLLIN | EPOLLHUP | EPOLLET;

    if (loop->keep_running)
        server->epoller->flag = loop->keep_running;

    add_chan_to_epoller(server->epoller, server->ch);
}

void remove_server_from_loop(liby_server *server) {
    assert(server && server->loop);
    liby_loop *loop = server->loop;
    mutex_lock(&(loop->servers_mutex));
    list_del(&(server->list));
    mutex_unlock(&(loop->servers_mutex));
    //remove_chan_from_epoller(server->epoller, server->listenfd);
}

void remove_client_from_loop(liby_client *client) {
    assert(client && client->loop);
    liby_loop *loop = client->loop;
    mutex_lock(&(loop->clients_mutex));
    list_del(&(client->list));
    mutex_unlock(&(loop->clients_mutex));
    //remove_chan_from_epoller(client->epoller, client->sockfd);
}

void remove_client_from_server(liby_client *client) {
    assert(client && client->server);
    liby_server *server = client->server;
    mutex_lock(&(server->clients_mutex));
    list_del(&(client->list));
    mutex_unlock(&(server->clients_mutex));
    //remove_chan_from_epoller(client->epoller, client->sockfd);
    //liby_client_destroy(client);
}


void liby_client_release_data(liby_client *client) {
    if (client) {
        if (client->release_data_func) {
            client->release_data_func(client->data);
            client->release_data_func = NULL;
        } else {
            free(client->data);
        }

        client->data = NULL;
    }
}

static void liby_server_acceptor(chan *c) {
    assert(c && c->p && c->event);
    liby_server *server = (liby_server *)(c->p);
    liby_loop *loop = server->loop;
    int listenfd = server->listenfd;

    if (c->event->events & EPOLLERR) {
        log_err("server error!\n");
        remove_server_from_loop(server);
        return;
    }

    while (1) {
        int clfd = accept(listenfd, NULL, NULL);
        if (clfd < 0) {
            if (errno == EAGAIN) {
                break;
            } else {
                log_err("accept error: %s\n", strerror(errno));
            }
        }

        liby_client *client = liby_client_init(clfd, loop);
        client->server = server;
        LOCK(server->clients_mutex);
        list_add_tail(&(client->list), &(server->clients.list));
        UNLOCK(server->clients_mutex);

        if (server->acceptor) {
            server->acceptor(client);
        }
    }
}

static void liby_client_handler(chan *c) {
    assert(c && c->p && c->event);
    //printf("handler was called!\n");
    liby_client *client = (liby_client *)(c->p);
    liby_server *server = client->server;
    liby_loop *loop = client->loop;

    if (c->event->events & EPOLLHUP || c->event->events & EPOLLERR ||
        c->event->events & EPOLLHUP) {
        liby_client_destroy(client);
    } else if (c->event->events & EPOLLIN) {
        read_message(client);
    } else if (c->event->events & EPOLLOUT) {
        write_message(client);
    }
}

static inline int liby_client_readable(liby_client *client) {
    return !(client->ch->event->events & EPOLLIN);
}

static inline int liby_client_writeable(liby_client *client) {
    return !(client->ch->event->events & EPOLLOUT);
}

static void read_message(liby_client *client) {
    assert(client && client->loop && client->epoller);
    liby_loop *loop = client->loop;
    liby_server *server = client->server;

    if (client->curr_read_task == NULL) {
        client->curr_read_task = pop_read_task_from_client(client);
        if (client->curr_read_task == NULL) {
            return;
        }
    }

    while (client->curr_read_task) {
        io_task *p = client->curr_read_task;

        while (1) {
            int ret =
                read(client->sockfd, p->buf + p->offset, p->size - p->offset);
            if (ret > 0) {
                p->offset += ret;
                if (p->offset >= p->min_except_bytes) {
                    if (p->handler) {
                        p->handler(client, p->buf, p->offset, 0);
                    }
                    break;
                }
            } else {
                if (ret != 0 && errno == EAGAIN) {
                    if (!liby_client_readable(client)) {
                        disable_read(client);
                    }
                } else {
                    int ec = errno;
                    if(ec == 0 && ret == 0) ec = -1;
                    if (p->handler) {
                        p->handler(client, p->buf, p->offset, ec);
                    }

                    liby_client_destroy(client);
                }

                return;
            }
        }

        client->curr_read_task = pop_read_task_from_client(client);
        io_task_destroy(p);
    }
    disable_read(client);
}

static void write_message(liby_client *client) {
    assert(client && client->loop && client->epoller);
    liby_loop *loop = client->loop;
    liby_server *server = client->server;

    if (client->curr_write_task == NULL) {
        client->curr_write_task = pop_write_task_from_client(client);
        if (client->curr_write_task == NULL) {
            return;
        }
    }

    while (client->curr_write_task) {
        io_task *p = client->curr_write_task;

        while (1) {
            int ret =
                write(client->sockfd, p->buf + p->offset, p->size - p->offset);
            if (ret > 0) {
                p->offset += ret;
                if (p->offset >= p->min_except_bytes) {
                    if (p->handler) {
                        p->handler(client, p->buf, p->offset, 0);
                    }
                    break;
                }
            } else {
                if (ret != 0 && errno == EAGAIN) {
                    if (!liby_client_writeable(client)) {
                        disable_write(client);
                    }
                } else {
                    int ec = errno;
                    if(ec == 0 && ret == 0) ec = -1;
                    if (p->handler) {
                        p->handler(client, p->buf, p->offset, ec);
                    }

                    liby_client_destroy(client);
                }

                return;
            }
        }

        client->curr_write_task = pop_write_task_from_client(client);
        io_task_destroy(p);
    }
    disable_write(client);
}

static void push_read_task_to_client(io_task *task, liby_client *client) {
    assert(task && client);
    LOCK(client->io_read_mutex);
    list_add_tail(&task->list, &client->read_list_head.list);
    UNLOCK(client->io_read_mutex);
}

static io_task *pop_read_task_from_client(liby_client *client) {
    assert(client);
    io_task *ret = NULL;
    LOCK(client->io_read_mutex);
    if (!list_empty(&(client->read_list_head.list))) {
        ret = list_entry(client->read_list_head.list.next, io_task, list);
        list_del(&(ret->list));
    }
    UNLOCK(client->io_read_mutex);
    return ret;
}

static void push_write_task_to_client(io_task *task, liby_client *client) {
    assert(task && client);
    LOCK(client->io_write_mutex);
    list_add_tail(&task->list, &client->write_list_head.list);
    UNLOCK(client->io_write_mutex);
}

static io_task *pop_write_task_from_client(liby_client *client) {
    assert(client);
    io_task *ret = NULL;
    LOCK(client->io_write_mutex);
    if (!list_empty(&(client->write_list_head.list))) {
        ret = list_entry(client->write_list_head.list.next, io_task, list);
        list_del(&(ret->list));
    }
    UNLOCK(client->io_write_mutex);
    return ret;
}

void liby_async_read_some(liby_client *client, char *buf, off_t buffersize,
                          handle_func handler) {
    assert(client);

    if (buf == NULL) {
        buf = safe_malloc(DEFAULT_BUFFER_SIZE);
        buffersize = DEFAULT_BUFFER_SIZE;
    }

    ALLOC(io_task, task);
    task->buf = buf;
    task->size = buffersize;
    task->offset = 0;
    task->min_except_bytes;
    task->handler = handler;

    push_read_task_to_client(task, client);
    enable_read(client);
}

void liby_async_read(liby_client *client, handle_func handler) {
    liby_async_read_some(client, NULL, 0, handler);
}

void liby_async_write_some(liby_client *client, char *buf, off_t buffersize,
                           handle_func handler) {
    assert(client && buffersize && buf);

    if (buf == NULL) {
        buf = safe_malloc(DEFAULT_BUFFER_SIZE);
        buffersize = DEFAULT_BUFFER_SIZE;
    }

    ALLOC(io_task, task);
    task->buf = buf;
    task->size = buffersize;
    task->offset = 0;
    task->min_except_bytes;
    task->handler = handler;

    push_write_task_to_client(task, client);
    enable_write(client);
}

void io_task_destroy(io_task *p) {
    assert(p);
    //free(p->buf);
    free(p);
}

void enable_read(liby_client *client) {
    assert(client && client->ch);
    client->ch->event->events |= (EPOLLIN);
    mod_chan_of_epoller(client->epoller, client->ch);
}

void disable_read(liby_client *client) {
    assert(client && client->ch);
    client->ch->event->events &= ~(EPOLLIN);
    mod_chan_of_epoller(client->epoller, client->ch);
}

void enable_write(liby_client *client) {
    assert(client && client->ch);
    client->ch->event->events |= (EPOLLOUT);
    mod_chan_of_epoller(client->epoller, client->ch);
}

void disable_write(liby_client *client) {
    assert(client && client->ch);
    client->ch->event->events &= ~(EPOLLOUT);
    mod_chan_of_epoller(client->epoller, client->ch);
}

int get_servers_of_loop(liby_loop *loop, liby_server **p) {
    assert(loop && p);

    int ret = 0;
    if (list_empty(&(loop->servers.list))) {
        *p = NULL;
    } else {
        int i = 0;
        struct list_head *pos = NULL;

        list_for_each(pos, &(loop->servers.list)) { ret++; }

        NALLOC(liby_server *, servers, ret);
        list_for_each(pos, &(loop->servers.list)) {
            servers[i++] = list_entry(pos, liby_server, list);
        }

        *p = *servers;
    }

out:
    return ret;
}

int get_clients_of_loop(liby_loop *loop, liby_client **p) {
    assert(loop && p);

    int ret = 0;
    if (list_empty(&(loop->clients.list))) {
        *p = NULL;
    } else {
        int i = 0;
        struct list_head *pos = NULL;

        list_for_each(pos, &(loop->clients.list)) { ret++; }

        NALLOC(liby_client *, clients, ret);
        list_for_each(pos, &(loop->clients.list)) {
            clients[i++] = list_entry(pos, liby_client, list);
        }

        *p = *clients;
    }

out:
    return ret;
}

int get_clients_of_server(liby_server *server, liby_client **p) {
    assert(server && p);

    int ret = 0;
    if (list_empty(&(server->clients.list))) {
        *p = NULL;
    } else {
        int i = 0;
        struct list_head *pos = NULL;

        list_for_each(pos, &(server->clients.list)) { ret++; }

        NALLOC(liby_client *, clients, ret);
        list_for_each(pos, &(server->clients.list)) {
            clients[i++] = list_entry(pos, liby_client, list);
        }

        *p = *clients;
    }

out:
    return ret;
}

int liby_connect_tcp(const char *path, const char *name) {
    return connect_tcp(path, name);
}

void set_connect_handler_for_client(liby_client *client,
                                    connect_func conn_func) {
    if (client) {
        client->conn_func = conn_func;
    }
}

void set_acceptor_for_server(liby_server *server, accept_func acceptor) {
    if (server != NULL) {
        server->acceptor = acceptor;
    }
}
