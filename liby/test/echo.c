#include "liby/liby.h"

void
write_all_handler(liby_client *client, char *buf, off_t length, int ec)
{
    free(buf);
    del_client_from_server(client, client->server);
}

void
read_all_handler(liby_client *client, char *buf, off_t length, int ec)
{
    if(ec) {
        fprintf(stderr, "%s\n", "error!");
        return;
    } else {
        liby_async_write_some(client, buf, length, write_all_handler);
    }
}

void
echo_aceptor(liby_client *client)
{
    liby_async_read(client, read_all_handler);
}

int main(int argc, char **argv)
{
    liby_server *echo_server = liby_server_init("echo", "9377");
    if(server == NULL) exit(1);

    epoller_t *loop = epoller_init(10240);
    if(loop == NULL) exit(2);

    set_acceptor_for_server(echo_aceptor, echo_server);

    add_server_to_epoller(echo_server, loop);

    loop->run_epoll_main_loop();

    liby_server_destroy(echo_server);
    epoller_destroy(loop);

    return 0;
}