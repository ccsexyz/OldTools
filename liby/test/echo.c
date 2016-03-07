#include "../src/liby.h"

void read_all_handler(liby_client *client, char *buf, off_t length, int ec);

void write_all_handler(liby_client *client, char *buf, off_t length, int ec) {
    free(buf);
    if (ec != 0) {
        //log_err("error");
        return;
    } else {
        liby_async_read(client, read_all_handler);
    }
}

void read_all_handler(liby_client *client, char *buf, off_t length, int ec) {
    if (ec != 0) {
        log_err("error");
        return;
    } else {
        // write(1, buf, length);
        liby_async_write_some(client, buf, length, write_all_handler);
    }
}

void echo_aceptor(liby_client *client) {
    //printf("echo_aceptor!\n");
    if (client == NULL) printf("client is NULL");
    liby_async_read(client, read_all_handler);
}

int main(int argc, char **argv) {
    liby_server *echo_server = liby_server_init("localhost", "9377", echo_aceptor);
    liby_loop *loop = liby_create_easy();
    //liby_loop *loop = liby_loop_create(1);
    add_server_to_loop(echo_server, loop);
    run_liby_main_loop(loop);
    return 0;
}
