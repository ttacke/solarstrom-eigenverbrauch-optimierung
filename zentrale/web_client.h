#pragma once
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <Regexp.h>
// https://stackoverflow.com/questions/34078497/esp8266-wificlient-simple-http-get
// https://arduinogetstarted.com/reference/serial-readbytes

namespace Local {
	class WebClient {
	protected:
		WiFiClient* wlan_client;
		MatchState match_state;
		int content_length = 0;
		char old_buffer[64];
		char search_buffer[128];

		bool _send_request(const char* host, const char* request_uri) {
			wlan_client->print("GET ");
			wlan_client->print(request_uri);
			wlan_client->print(" HTTP/1.1\r\nHost: ");
			wlan_client->print(host);
			wlan_client->print("\r\nConnection: close\r\n\r\n");

			int i = 0;
			while(!wlan_client->available()) {
				i++;
				if(i > 2000) {// 20s timeout
					return false;
				}
				delay(10);
			}
			return true;
		}

		void _read_content_length_header() {
			if(content_length == 0 && find_in_buffer((char*) "\r\nContent[-]Length: *([0-9]+)\r\n")) {
				content_length = atoi(finding_buffer);
			}
		}

		void _prepare_search_buffer() {
			int old_buffer_strlen = strlen(old_buffer);
			memcpy(search_buffer, old_buffer, old_buffer_strlen);
			memcpy(search_buffer + old_buffer_strlen, buffer, strlen(buffer) + 1);
		}

	public:
		char finding_buffer[65];
		char buffer[64];

		WebClient(WiFiClient& wlan_client): wlan_client(&wlan_client) {
		}

		bool find_in_buffer(char* regex) {
			match_state.Target(search_buffer);
			char result = match_state.Match(regex);
			if(result > 0) {
				match_state.GetCapture(finding_buffer, 0);
				return true;
			}
			return false;
		}

		void send_http_get_request(const char* host, const int port, const char* request_uri) {
			content_length = 0;
			buffer[0] = '\0';
			if(!wlan_client->connect(host, port)) {
				return;
			}
			if(!_send_request(host, request_uri)) {
				return;
			}

			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			old_buffer[0] = '\0';
			int max_read_size = sizeof(buffer) - 1;
			while(wlan_client->available()) {
				memcpy(old_buffer, buffer, strlen(buffer) + 1);

				int read_size = 0;
				bool content_start_reached = false;
				while(true) {
					wlan_client->readBytes(buffer + read_size, 1);
					read_size++;
					if(
						read_size >= 4
						&& strncmp(buffer + read_size - 4, "\r\n\r\n", 4) == 0
					) {
						content_start_reached = true;
						break;
					}
					if(
						read_size == max_read_size - 1
						||
						(
							read_size == max_read_size - (1 + 4)
							&& !strncmp(buffer + read_size, "\r", 1) == 0
							&& !strncmp(buffer + read_size, "\n", 1) == 0
						)
					) {
						break;
					}
				}
				std::fill(buffer + read_size, buffer + sizeof(buffer), 0);// Rest immer leeren
				_prepare_search_buffer();
				_read_content_length_header();
				if(content_start_reached) {
					old_buffer[0] = '\0';
					_prepare_search_buffer();
					return;
				}
			}
		}

		bool read_next_block_to_buffer() {
			if(content_length <=0) {
				return false;
			}

			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			if(wlan_client->available()) {
				memcpy(old_buffer, buffer, strlen(buffer) + 1);
				int read_size = wlan_client->readBytes(
					buffer,
					std::min((size_t) content_length, (size_t) (sizeof(buffer) - 1))
				);
				std::fill(buffer + read_size, buffer + sizeof(buffer), 0);// Rest immer leeren
				_prepare_search_buffer();
				content_length -= strlen(buffer);
				return true;
			}
			buffer[0] = '\0';
			content_length = 0;
			return false;
		}
	};
}
