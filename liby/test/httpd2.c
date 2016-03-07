#include <stdio.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include "http_parser.h"
#include "liby.h"

void write_header_handler(liby_client *client, char *buf, off_t length,
                          int ec) {
    if (!ec) {
        off_t *data = (off_t *)(client->data);
        off_t offset = 0;
        sendfile(client->sockfd, data[0], &offset, data[1]);
        free(buf);
        close(data[0]);
    }
    del_client_from_server(client, client->server);
}

void read_all_handler(liby_client *client, char *buf, off_t length, int ec) {
    if (ec) {
        fprintf(stderr, "error\n");
    } else {
        request_t *r = http_request_parse(buf);

        if (r->path_string && *(r->path_string)) {
            char *path = r->path_string;

            struct stat file;
            if (!stat(path, &file)) {
                off_t *data = malloc(sizeof(off_t) * 2);
                data[0] = open(path, O_RDONLY, 0);
                data[1] = file.st_size;
                if (data[0] >= 0) {
                    sprintf(
                        buf,
                        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
                    set_data_of_client_with_free(client, data);
                    liby_async_write_some(client, buf, strlen(buf),
                                          write_header_handler);
                    return;
                } else {
                    free(data);
                }
            }
        }
        http_request_destroy(r);
    }
    del_client_from_server(client, client->server);
}

void httpd_accepter(liby_client *client) {
    if (client) liby_async_read(client, read_all_handler);
}

int main(void) {
    liby_server *httpd_server = liby_server_init("localhost", "8000");
    if (httpd_server == NULL) exit(1);

    epoller_t *loop = epoller_init(10240);
    if (loop == NULL) exit(2);

    set_acceptor_for_server(httpd_server, httpd_accepter);
    add_server_to_epoller(httpd_server, loop);

    run_epoll_main_loop(loop, get_default_epoll_handler());

    liby_server_destroy(httpd_server);
    epoller_destroy(loop);
    return 0;
}
