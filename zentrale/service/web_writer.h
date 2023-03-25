#pragma once

namespace Local::Service {
	class WebWriter {
	protected:
		Local::Service::Webserver& webserver;
		char buffer[2048];
		int buffer_offset = 0;

	public:
		WebWriter(
			Local::Service::Webserver& webserver
		): webserver(webserver) {
		}

		void init_for_write(int return_code, const char* content_type) {
			webserver.server.setContentLength(CONTENT_LENGTH_UNKNOWN);
			webserver.server.send(return_code, content_type, "");
			std::fill(buffer, buffer + sizeof(buffer), 0);
			buffer_offset = 0;
		}

		void init_for_write(int return_code, const char* content_type, int content_length) {
			webserver.server.setContentLength(content_length);
			webserver.server.send(return_code, content_type, "");
			std::fill(buffer, buffer + sizeof(buffer), 0);
			buffer_offset = 0;
		}

		// Bug, der eigentlich gefixt sein sollte: https://github.com/esp8266/Arduino/issues/6877
		// Doku zu ChunkedEncoding https://en.wikipedia.org/wiki/Chunked_transfer_encoding
		void write(char* string, int string_length) {
			int buffer_space_length = sizeof(buffer) - buffer_offset - 1;
			int read_string_till = std::min(string_length, buffer_space_length);
			memcpy(buffer + buffer_offset, string, read_string_till);
			if(read_string_till < string_length) {
				webserver.server.sendContent(buffer);
				std::fill(buffer, buffer + sizeof(buffer), 0);
				buffer_offset = 0;
				return write(string + read_string_till, string_length - read_string_till);
			}
			buffer_offset += string_length;
		}

		void flush_write_buffer() {
			webserver.server.sendContent(buffer);
			std::fill(buffer, buffer + sizeof(buffer), 0);
			buffer_offset = 0;
		}
	};
}
