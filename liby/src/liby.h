#ifndef LIBY_LIBY_H
#define LIBY_LIBY_H

#include "base.h"
#include "epoll.h"

#define DEFAULT_BUFFER_SIZE (4096)

typedef struct liby_client_ liby_client;
typedef struct liby_loop_ liby_loop;
typedef struct liby_server_ liby_server;
typedef void (*accept_func)(liby_client *);
typedef void (*connect_func)(liby_client *);
typedef void (*handle_func)(liby_client *, char *, off_t, int);
typedef void (*release_func)(void *);

typedef struct io_task_ {
    char *buf;
    off_t size;
    off_t offset;
    off_t min_except_bytes;
    handle_func handler;

    struct list_head list;
} io_task;

typedef struct liby_client_ {
    int sockfd;
    epoller_t *epoller;
    chan *ch;
    liby_loop *loop;
    liby_server *server;

    mutex_t io_read_mutex;
    mutex_t io_write_mutex;
    io_task read_list_head, write_list_head;
    io_task *curr_read_task, *curr_write_task;

    void *data;
    release_func release_data_func;
    connect_func conn_func;
    struct epoll_event event;

    struct list_head list;
} liby_client;

typedef struct liby_server_ {
    int listenfd;
    char *server_path;
    char *server_port;
    epoller_t *epoller;
    liby_loop *loop;
    chan *ch;
    struct epoll_event event;
    accept_func acceptor;

    mutex_t clients_mutex;
    liby_client clients;
    struct list_head list;
} liby_server;

typedef struct liby_loop_ {
    int maxfds;
    int nthreads;
    int keep_running;
    pthread_t *tids;
    epoller_t **epollers;
    liby_client clients;
    mutex_t clients_mutex;
    liby_server servers;
    mutex_t servers_mutex;
} liby_loop;

int liby_connect_tcp(const char *path, const char *name);

liby_loop *liby_create_easy(void);

liby_loop *liby_loop_create(int nthreads);

void run_liby_main_loop(liby_loop *loop);

void liby_loop_destroy(liby_loop *loop);

liby_client *liby_client_init(int clfd, liby_loop *loop);

liby_server *liby_server_init(const char *server_path, const char *server_port,
                              accept_func acceptor);

void liby_server_destroy(liby_server *server);

void add_server_to_loop(liby_server *server, liby_loop *loop);

void remove_server_from_loop(liby_server *server);

void remove_client_from_loop(liby_client *client);

void remove_client_from_server(liby_client *client);

void liby_client_destroy(liby_client *client);

void liby_client_release_data(liby_client *client);

void liby_async_read_some(liby_client *client, char *buf, off_t buffersize,
                          handle_func handler);

void liby_async_read(liby_client *client, handle_func handler);

void liby_async_write_some(liby_client *client, char *buf, off_t buffersize,
                           handle_func handler);

void enable_read(liby_client *client);

void disable_read(liby_client *client);

void enable_write(liby_client *client);

void disable_write(liby_client *client);

int get_servers_of_loop(liby_loop *loop, liby_server **p);

int get_clients_of_loop(liby_loop *loop, liby_client **p);

int get_clients_of_server(liby_server *server, liby_client **p);

void set_connect_handler_for_client(liby_client *client,
                                    connect_func conn_func);

void set_acceptor_for_server(liby_server *server, accept_func acceptor);

#endif
