#include "liby.h"

void read_all_handler(liby_client *client, char *buf, off_t length, int ec);

void
write_all_handler(liby_client *client, char *buf, off_t length, int ec)
{
    free(buf);

    if(ec) {
        fprintf(stderr, "%s\n", "error!");
        del_client_from_server(client, client->server);
        return;
    } else {
        liby_async_read(client, read_all_handler);
    }
}

void
read_all_handler(liby_client *client, char *buf, off_t length, int ec)
{
    if(ec) {
        fprintf(stderr, "%s\n", "error!");
        del_client_from_server(client, client->server);
        return;
    } else {
        //write(1, buf, length);
        liby_async_write_some(client, buf, length, write_all_handler);
    }
}

void
echo_aceptor(liby_client *client)
{
    printf("echo_aceptor!\n");
    if(client == NULL) printf("client is NULL");
    liby_async_read(client, read_all_handler);
}

int main(int argc, char **argv)
{
    liby_server *echo_server = liby_server_init("localhost", "9377");
    if(echo_server == NULL) exit(1);

    epoller_t *loop = epoller_init(10240);
    if(loop == NULL) exit(2);

    set_acceptor_for_server(echo_server, echo_aceptor);

    add_server_to_epoller(echo_server, loop);

    run_epoll_main_loop(loop, get_default_epoll_handler());

    liby_server_destroy(echo_server);
    epoller_destroy(loop);

    return 0;
}
