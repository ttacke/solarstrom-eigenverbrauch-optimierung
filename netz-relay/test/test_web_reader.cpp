#include "mock_arduino.h"
#include "mock_regexp.h"
#include "../web_reader.h"
#include <cassert>
#include <cstdio>

void test_find_in_buffer_findet_content() {
    WiFiClient client;
    client.set_response(
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "\r\n"
        "ok"
    );
    Local::WebReader reader(client);
    reader.send_http_get_request("test", 80, "/");
    reader.read_next_block_to_buffer();
    assert(reader.find_in_buffer((char*)"ok"));
    printf("test_find_in_buffer_findet_content OK\n");
}

int main() {
    test_find_in_buffer_findet_content();
}
