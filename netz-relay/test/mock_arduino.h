#pragma once
#include <cstring>
#include <algorithm>
#include <string>

// Arduino Stubs
void delay(int) {}
void yield() {}

struct SerialMock {
    void begin(int) {}
    void print(const char*) {}
    void print(char) {}
    void print(int) {}
    void println(const char*) {}
    void println(int) {}
} Serial;

// WiFiClient Mock
class WiFiClient {
    std::string response;
    size_t pos = 0;
public:
    void set_response(const char* r) { response = r; pos = 0; }
    bool connect(const char*, int) { return true; }
    void print(const char*) {}
    int available() { return pos < response.size() ? response.size() - pos : 0; }
    int readBytes(char* buf, int len) {
        int to_read = std::min((size_t)len, response.size() - pos);
        memcpy(buf, response.data() + pos, to_read);
        pos += to_read;
        return to_read;
    }
};
