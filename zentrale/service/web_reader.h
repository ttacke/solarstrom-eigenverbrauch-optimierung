#pragma once
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <Regexp.h>
// https://stackoverflow.com/questions/34078497/esp8266-wificlient-simple-http-get
// https://arduinogetstarted.com/reference/serial-readbytes

namespace Local::Service {
	class WebReader {
	protected:
		WiFiClient& wlan_client;
		MatchState match_state;

		int content_length = 0;
		char old_buffer[64];
		char search_buffer[128];
		bool is_chunked = false;
		bool debug = false;

		bool _send_request(
			const char* host, const char* request_uri, int timeout_in_hundertstel_s
		) {
			wlan_client.print("GET ");
			wlan_client.print(request_uri);
			wlan_client.print(" HTTP/1.1\r\nHost: ");
			wlan_client.print(host);
			wlan_client.print("\r\nConnection: close\r\n\r\n");

			int i = 0;
			while(!wlan_client.available()) {
				i++;
				if(i > timeout_in_hundertstel_s) {
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

		bool _has_chunk_header() {
			if(is_chunked) {
				return true;
			}
			if(find_in_buffer((char*) "\r\nTransfer[-]Encoding: *chunked\r\n")) {
				return true;
			}
			return false;
		}

		void _prepare_search_buffer() {
			int old_buffer_strlen = strlen(old_buffer);
			memcpy(search_buffer, old_buffer, old_buffer_strlen);
			memcpy(search_buffer + old_buffer_strlen, buffer, strlen(buffer) + 1);
		}

		void _read_body_content_to_buffer() {
			int read_size = wlan_client.readBytes(
				buffer,
				std::min((size_t) content_length, (size_t) (sizeof(buffer) - 1))
			);
			std::fill(buffer + read_size, buffer + sizeof(buffer), 0);// Rest immer leeren
			if(debug) {
				Serial.print(buffer);
				Serial.print("|");
			}
			_prepare_search_buffer();
			content_length -= strlen(buffer);
			if(content_length == 0 && is_chunked) {
				_read_next_chunk_content_length();
				if(debug) {
					Serial.print("[NextChunk:");
					Serial.print(content_length);
					Serial.print("]");
				}
			}
		}

	public:
		char finding_buffer[65];
		char buffer[64];
		int default_timeout_in_hundertstel_s = 2000;
		int kurzer_timeout_in_hundertstel_s = 500;

		WebReader(WiFiClient& wlan_client): wlan_client(wlan_client) {
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

		bool send_http_get_request(const char* host, const int port, const char* request_uri) {
			return send_http_get_request(
				host, port, request_uri, default_timeout_in_hundertstel_s
			);
		}

		bool send_http_get_request(
			const char* host, const int port, const char* request_uri, int timeout_in_hundertstel_s
		) {
			content_length = 0;
			old_buffer[0] = '\0';
			buffer[0] = '\0';
			is_chunked = false;
			if(!wlan_client.connect(host, port)) {
				return false;
			}
			if(!_send_request(host, request_uri, timeout_in_hundertstel_s)) {
				return false;
			}

			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			int max_buffer_offset = sizeof(buffer) - 1;
			if(debug) {
				Serial.print("ReadWeb\n----\n");
			}
			while(wlan_client.available()) {
				memcpy(old_buffer, buffer, strlen(buffer) + 1);
				std::fill(buffer, buffer + sizeof(buffer), 0);// Reset

				int buffer_offset = 0;
				bool content_start_reached = false;
				while(true) {
					wlan_client.readBytes(buffer + buffer_offset, 1);// Liest 1 Byte in den buffer, beginnend beim offset
					buffer_offset++;
					if(buffer_offset < 4) {
						_prepare_search_buffer();
						if(find_in_buffer((char*) "\r\n\r\n")) {
							content_start_reached = true;
							break;
						}
					} else if (// geiches wie oben drueber, aber schneller
						strncmp(buffer + buffer_offset - 4, "\r\n\r\n", 4) == 0
					) {
						content_start_reached = true;
						break;
					} else if(// Wenn buffer voll
						buffer_offset == max_buffer_offset
					) {
						break;
					}
				}
				std::fill(buffer + buffer_offset, buffer + sizeof(buffer), 0);// Rest immer leeren
				if(debug) {
					Serial.print(buffer);
					Serial.print("|");
				}

				_prepare_search_buffer();
				_read_content_length_header();
				if(!content_length) {
					is_chunked = _has_chunk_header();
				}
				if(content_start_reached) {
					if(debug) {
						Serial.println("\n---\n->end");
					}
					if(is_chunked) {
						_read_next_chunk_content_length();
					}
					old_buffer[0] = '\0';
					buffer[0] = '\0';
					if(debug) {
						Serial.println(is_chunked ? "Chunked" : "NoChunk");
						Serial.println("Content-Length");
						Serial.println(content_length);
					}
					return true;
				}
			}
			return false;
		}

		void _read_next_chunk_content_length() {
			int buffer_offset = 0;
			buffer[0] = '\0';
			content_length = 0;
			int max_buffer_offset = sizeof(buffer) - 1;
			while(true) {
				wlan_client.readBytes(buffer + buffer_offset, 1);
				buffer_offset++;
				if(// Lies den Start des Chunks und ermittle die Laenge
					buffer_offset > 2 // min 1 Byte muss da noch zusaetzlich sein
					&& strncmp(buffer + buffer_offset - 2, "\r\n", 2) == 0
				) {
					std::fill(buffer + buffer_offset - 2, buffer + sizeof(buffer), 0); // ende abschneiden
					if(debug) {
						Serial.print("[Chunk:");
						Serial.print(buffer_offset);
						Serial.print(",");
						Serial.print(buffer);
						Serial.print("]");
					}
					content_length = content_length + strtoul(buffer, 0, 16);
					break;
				} else if (buffer_offset == max_buffer_offset) {
					Serial.println("Error: no chunk found");
					content_length = 0;
					break;
				}
			}
		}

		bool read_next_block_to_buffer() {
			if(content_length <= 0) {
				return false;
			}
			if(wlan_client.available()) {
				memcpy(old_buffer, buffer, strlen(buffer) + 1);
				_read_body_content_to_buffer();
				return true;
			}
			buffer[0] = '\0';
			content_length = 0;
			return false;
		}
	};
}
