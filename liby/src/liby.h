#include "epoll.h"
#include "socket.h"

typedef struct liby_server_ {
    char *server_name;
    char *server_port;
    int listenfd;

    epoller_t *loop;
} liby_server;

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
