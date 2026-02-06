#pragma once
#include <cstring>
#include <cstddef>
#include <string>

class WiFiClient {
	const char* response_data = nullptr;
	size_t response_len = 0;
	size_t read_pos = 0;
	std::string sent_data;

public:
	// Test-Setup Methoden
	void set_response(const char* data, size_t len) {
		response_data = data;
		response_len = len;
		read_pos = 0;
	}

	void set_response(const char* data) {
		set_response(data, strlen(data));
	}

	const std::string& get_sent_data() const {
		return sent_data;
	}

	void reset() {
		response_data = nullptr;
		response_len = 0;
		read_pos = 0;
		sent_data.clear();
	}

	// WiFiClient Interface
	bool connect(const char*, int) {
		return true;
	}

	int available() {
		return (int)(response_len - read_pos);
	}

	size_t readBytes(char* buffer, size_t length) {
		if (read_pos >= response_len) return 0;
		size_t to_read = std::min(length, response_len - read_pos);
		memcpy(buffer, response_data + read_pos, to_read);
		read_pos += to_read;
		return to_read;
	}

	size_t print(const char* s) {
		sent_data += s;
		return strlen(s);
	}

	size_t print(char c) {
		sent_data += c;
		return 1;
	}

	void stop() {}
};
