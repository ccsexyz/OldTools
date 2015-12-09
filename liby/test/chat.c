#include "liby.h"

liby_server *echo_server;

void
read_all_handler(liby_client *client, char *buf, off_t length, int ec)
{
    if(ec) {
        fprintf(stderr, "%s\n", "error!");
        del_client_from_server(client, client->server);
        return;
    } else {
        //write(1, buf, length);
        liby_client *head = get_clients_of_server(echo_server);
        while(head) {
            if(head != client)
                liby_async_write_some(head, buf, length, NULL);
            head = head->next;
        }
        liby_async_read(client, read_all_handler);
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
    echo_server = liby_server_init("localhost", "9377");
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
