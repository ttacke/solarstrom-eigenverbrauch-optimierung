#pragma once

namespace Local::Service {
	class WebWriter {
	protected:
		Local::Service::Webserver& webserver;
		char buffer[2048];
		int buffer_offset = 0;
		char chunk_head[6];

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
			_init_buffer();
		}

		void _init_buffer() {
			std::fill(buffer, buffer + sizeof(buffer), 0);
			sprintf(buffer, "xxx\r\n\0");// Chunk-Head-Platzhalter
			buffer_offset = strlen(buffer);
		}

		// Bug, der eigentlich gefixt sein sollte: https://github.com/esp8266/Arduino/issues/6877
		// Doku zu ChunkedEncoding https://en.wikipedia.org/wiki/Chunked_transfer_encoding
		// Bug umgangen -> Chunked selber implementiert
		// (der originale Code killt die Verbindung oder fuegt ChunkedHeader im Content ein)
		void write(char* string, int string_length) {
			int free_bytes_at_the_end_of_buffer = sizeof(buffer) - buffer_offset - 3;// \0 + Platz fuer Chunk-Foot
			int read_string_till = std::min(string_length, free_bytes_at_the_end_of_buffer);
			memcpy(buffer + buffer_offset, string, read_string_till);
			if(read_string_till < string_length) {
				_send_buffer_to_client();
				_init_buffer();
				return write(string + read_string_till, string_length - read_string_till);
			}
			buffer_offset += string_length;
		}

		void _send_buffer_to_client() {
			sprintf(chunk_head, "%03x\r\n\0", strlen(buffer) - 5);
			memcpy(buffer, chunk_head, 5);// Chunk-Head
			memcpy(buffer + strlen(buffer), "\r\n\0", 3);// Chunk-Foot
			webserver.server.sendContent(buffer);
			delay(10);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
		}

		void flush_write_buffer_and_close_transfer() {
			if(strlen(buffer) > 0) {
				_send_buffer_to_client();
			}
			sprintf(buffer, "0\r\n\r\n\0");
			webserver.server.sendContent(buffer);
			_init_buffer();
		}
	};
}
