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

void test_find_in_buffer_findet_content_ueber_chunks() {
    WiFiClient client;
    client.set_response(
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "1\r\n"
        "o"
        "1\r\n"
        "k"
        "0\r\n"
        "\r\n"
    );
    Local::WebReader reader(client);
    reader.send_http_get_request("test", 80, "/");
    reader.read_next_block_to_buffer();
    reader.read_next_block_to_buffer();
    assert(reader.find_in_buffer((char*)"ok"));
    printf("test_find_in_buffer_findet_content_ueber_chunks OK\n");
}

int main() {
    test_find_in_buffer_findet_content();
    test_find_in_buffer_findet_content_ueber_chunks();
}
