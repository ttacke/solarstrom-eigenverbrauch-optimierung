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

		void init_for_write(const char* content_type, int content_length) {
			webserver.server.setContentLength(content_length);
			webserver.server.send(200, content_type, "");
			_init_buffer();
		}

		void _init_buffer() {
			std::fill(buffer, buffer + sizeof(buffer), 0);
			buffer_offset = 0;
		}

		void write(char* string, int string_length) {
			int free_bytes_at_the_end_of_buffer = sizeof(buffer) - buffer_offset - 1;
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
			webserver.server.sendContent(buffer, strlen(buffer));
			delay(10);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
		}

		void flush_write_buffer_and_close_transfer() {
			if(strlen(buffer) > 0) {
				_send_buffer_to_client();
			}
			_init_buffer();
		}
	};
}
