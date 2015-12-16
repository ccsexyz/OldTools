#include <stdio.h>
#include "http_parser.h"
#define TEST_HTTP_REQUEST "GET /HELLO?this%20is%20query%20string HTTP/1.1\r\nAccept: text\\plain\r\nAccept2: Nothing\r\n\r\n"

int main(void)
{
    request_t *r = http_request_parse(TEST_HTTP_REQUEST);
    print_http_request(r);
    http_request_destroy(r);
    return 0;
}

