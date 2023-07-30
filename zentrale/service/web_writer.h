#pragma once

namespace Local::Service {
	class WebWriter {
	protected:
		Local::Service::Webserver& webserver;
		char buffer[2048];
		int buffer_offset = 0;
		char chunk_buffer[16];

	public:
		WebWriter(
			Local::Service::Webserver& webserver
		): webserver(webserver) {
		}

		void init_for_write(const char* content_type) {
			sprintf(
				buffer,
				"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nContent-Type: %s\r\n\r\n\0",
				content_type
			);
			webserver.server.sendContent(buffer);
			std::fill(buffer, buffer + sizeof(buffer), 0);
			buffer_offset = 0;
		}

		// Bug, der eigentlich gefixt sein sollte: https://github.com/esp8266/Arduino/issues/6877
		// Doku zu ChunkedEncoding https://en.wikipedia.org/wiki/Chunked_transfer_encoding
		// Bug umgangen -> Chunked selber implementiert
		// (der originale Code killt die Verbindung oder fuegt ChunkedHeader im Content ein)
		void write(char* string, int string_length) {
			int buffer_space_length = sizeof(buffer) - buffer_offset - 1;
			int read_string_till = std::min(string_length, buffer_space_length);
			memcpy(buffer + buffer_offset, string, read_string_till);
			if(read_string_till < string_length) {
				_send_buffer_to_client();
				std::fill(buffer, buffer + sizeof(buffer), 0);
				buffer_offset = 0;
				return write(string + read_string_till, string_length - read_string_till);
			}
			buffer_offset += string_length;
		}

		void _send_buffer_to_client() {
			sprintf(chunk_buffer, "%x\r\n\0", strlen(buffer));
			webserver.server.sendContent(chunk_buffer);
			webserver.server.sendContent(buffer);
			sprintf(chunk_buffer, "\r\n\0");
			webserver.server.sendContent(chunk_buffer);
		}

		void flush_write_buffer_and_close_transfer() {
			if(strlen(buffer) > 0) {
				_send_buffer_to_client();
			}
			sprintf(chunk_buffer, "0\r\n\r\n\0");
			webserver.server.sendContent(chunk_buffer);
			std::fill(buffer, buffer + sizeof(buffer), 0);
			buffer_offset = 0;
		}
	};
}
