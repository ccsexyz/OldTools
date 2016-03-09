#include "../src/liby.h"

void write_all_handler(liby_client *client, char *buf, off_t length, int ec) {
    log_info("write all!");
    free(buf);
}

void read_all_handler(liby_client *client, char *buf, off_t length, int ec) {
    if (!ec) {
        log_info("recv %s", buf);
        liby_client **clients;
        int n = get_clients_of_server(client->server, &clients);
        log_info("n = %d", n);
        for (int i = 0; i < n; i++) {
            char *mbuf = malloc(length);
            memcpy(mbuf, buf, length);
            if (clients[i] != client)
                liby_async_write_some(clients[i], mbuf, length,
                                      write_all_handler);
        }
        free(clients);
        liby_async_read(client, read_all_handler);
    } else {
        log_err("read");
    }
    free(buf);
}

void chat_acceptor(liby_client *client) {
    assert(client);
    liby_async_read(client, read_all_handler);
}

int main(int argc, char **argv) {
    liby_server *chat_server =
        liby_server_init("0.0.0.0", "9377", chat_acceptor);
    liby_loop *loop = liby_create_easy();
    add_server_to_loop(chat_server, loop);
    run_liby_main_loop(loop);
    return 0;
}
