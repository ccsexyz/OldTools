#include "../src/http_parser.h"
#include "../src/liby.h"
#include <stdio.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

void write_header_handler(liby_client *client, char *buf, off_t length,
                          int ec) {

    off_t *data = (off_t *)(liby_client_get_data(client));
    off_t offset = 0;

    if (ec < 0) {
        log_err("client quit with ec %d", ec);
    } else if (ec > 0) {
        log_err("httpd");
    } else {
        sendfile(client->sockfd, data[0], &offset, data[1]);
        close(data[0]);
        liby_destroy_client_later(client);
    }

    free(buf);
}

void read_all_handler(liby_client *client, char *buf, off_t length, int ec) {
    int success = 0;

    if (ec == -1) {
        log_info("client quit in the read_all_handler");
    } else if (ec > 0) {
        log_err("httpd");
    } else {
        request_t *r = http_request_parse(buf);

        if (r->path_string && *(r->path_string)) {
            char *path = r->path_string;
            log_info("try to open file %s", r->path_string);
            struct stat file;
            if (!stat(path, &file)) {
                log_info("success");
                off_t *data = malloc(sizeof(off_t) * 2);
                data[0] = open(path, O_RDONLY, 0);
                data[1] = file.st_size;
                if (data[0] >= 0) {
                    sprintf(
                        buf,
                        "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\n\r\n");
                    liby_client_set_data_with_free(client, data);
                    liby_async_write_some(client, buf, strlen(buf),
                                          write_header_handler);
                    success = 1;
                } else {
                    log_err("open file %s error", path);
                    free(data);
                }
            } else {
                log_err("stat");
            }
        } else {
            log_info("http request parse error!");
        }
        http_request_destroy(r);
    }

    if (success == 0) {
        free(buf);
        liby_destroy_client_later(client);
    }
}

void httpd_accepter(liby_client *client) {
    assert(client);
    liby_async_read(client, read_all_handler);
}

int main(void) {
    liby_server *httpd_server =
        liby_server_init("0.0.0.0", "8000", httpd_accepter);
    liby_loop *loop = liby_create_easy();
    add_server_to_loop(httpd_server, loop);
    run_liby_main_loop(loop);
    return 0;
}
