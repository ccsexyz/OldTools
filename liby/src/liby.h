#ifndef LIBY_LIBY_H
#define LIBY_LIBY_H

#include "base.h"
#include "epoll.h"
#include "socket.h"

#define DEFAULT_BUFFER_SIZE (4096)

typedef struct liby_client_ liby_client;

typedef void (*accept_func)(liby_client *);
typedef void (*hook_func)(void *);
typedef void (*handle_func)(liby_client *, char *, off_t, int);

typedef struct io_task_ {
    char *buf;
    off_t size;
    off_t offset;
    off_t min_except_bytes;
    handle_func handler;   //be set to NULL if it does not need any handler

    struct io_task_ *next, *prev;
} io_task;

typedef struct liby_server_ {
    int type;
    char *server_path;
    char *server_port;
    int listenfd;

    char *buf;
    off_t buffersize;
    int buf_allocate_flag;

    epoller_t *loop;

    accept_func acceptor;
    handle_func read_complete_handler;
    handle_func write_complete_handler;

    liby_client *head, *tail;
} liby_server;

typedef struct liby_client_ {
    int type;
    int sockfd;
    int is_created_by_server;

    char *buf;
    off_t buffersize;
    int buf_allocate_flag;

    io_task *read_list_head, *read_list_tail, *curr_read_task;
    io_task *write_list_head, *write_list_tail, *curr_write_task;
    mutex_t io_read_mutex;
    mutex_t io_write_mutex;

    liby_server *server;
    epoller_t *loop;
    struct liby_client_ *next, *prev;
} liby_client;

//liby_server *liby_server_init(const char *server_name, const char *server_port);

void liby_server_destroy(liby_server *server);

void add_server_to_epoller(liby_server *server, epoller_t *loop);

liby_server *liby_server_init(const char *server_path, const char *server_port);

void liby_server_destroy(liby_server *server);

void add_server_to_epoller(liby_server *server, epoller_t *loop);

void handle_epoll_event(epoller_t *loop, int n);

void epoll_acceptor(liby_server *server);

liby_client *liby_client_init_by_server(int fd, liby_server *server);

liby_client *liby_client_init(int fd, epoller_t *loop);

void liby_client_release(liby_client *c);

void add_client_to_server(liby_client *client, liby_server *server);

void del_client_from_server(liby_client *client, liby_server *server);

void read_message(liby_client *client);
void write_message(liby_client *client);

void push_io_task_to_client(io_task *task, liby_client *client, int type); //type == true read type == false write

void push_io_task(io_task **head, io_task **tail, io_task *p);

io_task *pop_io_task(io_task **head, io_task **tail);

io_task *pop_io_task_from_client(liby_client *client, int type); // true for read, false for write

void liby_async_read_some(liby_client *client, char *buf, off_t buffersize, handle_func handler);

void liby_async_read(liby_client *client, handle_func handler);

//void liby_async_connect_tcp(liby_client *client, const char *host, const char *port);

int liby_sync_connect_tcp(const char *host, const char *port);

void liby_async_write_some(liby_client *client, char *buf, off_t buffersize, handle_func handler);

void set_write_complete_handler_for_server(handle_func handler, liby_server *server);

void set_read_complete_handler_for_server(handle_func handler, liby_server *server);

void set_acceptor_for_server(accept_func acceptor, liby_server *server);

void set_buffer_for_server(liby_server *server, char *buf, off_t buffersize);

void set_default_buffer_for_server(liby_server *server);

char *get_server_buffer(liby_server *server);

off_t get_server_buffersize(liby_server *server);

void set_buffer_for_client(liby_client *client, char *buf, off_t buffersize);

void set_default_buffer_for_client(liby_client *client);

char *get_client_buffer(liby_client *client);

off_t get_client_buffersize(liby_client *client);

epoll_event_handler get_default_epoll_handler(void);

void add_client_to_epoller(liby_client *client, epoller_t *loop);
#endif
