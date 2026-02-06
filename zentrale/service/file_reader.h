#pragma once
#include <SPI.h>
#include <SD.h>
#include <Regexp.h>

namespace Local::Service {
	class FileReader {
	protected:
		bool init = false;
		File fh;
		MatchState match_state;
		int offset = 0;
		char old_buffer[128];
		char search_buffer[256];
		int buffer_len = 0;
		int old_buffer_len = 0;
		int next_capture_group;

		bool _init() {
			if(!init) {
				if (SD.begin(D2)) {
					init = true;
				} else {
					Serial.println("SD-Card initialization failed!");
					return false;
				}
			}
			return true;
		}

		void _prepare_search_buffer() {
			memcpy(search_buffer, old_buffer, old_buffer_len);
			memcpy(search_buffer + old_buffer_len, buffer, buffer_len);
			search_buffer[old_buffer_len + buffer_len] = '\0';
		}

		bool _read_next_block_to_buffer() {
			fh.seek(offset);
			int read_size = std::min(sizeof(buffer) - 1, fh.size() - offset);
			if(read_size == 0) {
				std::fill(buffer, buffer + sizeof(buffer), 0);
				buffer_len = 0;
				return false;
			}

			old_buffer_len = buffer_len;
			memcpy(old_buffer, buffer, buffer_len);
			fh.read((uint8_t*) buffer, read_size);
			buffer_len = read_size;
			std::fill(buffer + buffer_len, buffer + sizeof(buffer), 0);// Rest immer leeren
			offset += read_size;
			return true;
		}

	public:
		char finding_buffer[129];
		char buffer[128];

		void close_file() {
			fh.close();
		}

		bool open_file_to_read(const char* filename) {
			if(!_init() || SD.exists(filename) != 1) {
				return false;
			}
			offset = 0;
			buffer[0] = '\0';
			buffer_len = 0;
			old_buffer_len = 0;
			fh = SD.open(filename);
			return true;
		}

		int get_file_size() {
			return fh.size();
		}

		bool read_next_line_to_buffer() {
			if(!_read_next_block_to_buffer()) {
				return false;
			}
			for(int i = 0; i < buffer_len + 1; i++) {
				if(buffer[i] == '\n') {
					offset = offset - buffer_len + i + 1;// Zurueckspuhlen zum ersten Zeilenumbruch
					std::fill(buffer + i, buffer + sizeof(buffer), 0);// Rest immer leeren
					buffer_len = i;
					break;
				}
			}
			std::fill(old_buffer, old_buffer + sizeof(old_buffer), 0);// Leeren; nur buffer wird genutzt
			old_buffer_len = 0;
			_prepare_search_buffer();
			return true;
		}

		bool read_next_block_to_buffer() {
			if(!_read_next_block_to_buffer()) {
				return false;
			}
			_prepare_search_buffer();
			return true;
		}

		bool find_in_buffer(char* regex) {
			next_capture_group = 0;
			std::fill(finding_buffer, finding_buffer + sizeof(finding_buffer), 0);// zur Sicherheit
			match_state.Target(search_buffer);
			match_state.Match(regex);
			return fetch_next_finding();
		}

		bool fetch_next_finding() {
			if(next_capture_group < match_state.level) {
				if(match_state.capture_len(next_capture_group) < (int) sizeof(finding_buffer)) {
					match_state.GetCapture(finding_buffer, next_capture_group);
					next_capture_group++;
					return true;
				}
				// Capture zu lang - ueberspringe diese Gruppe
				next_capture_group++;
			}
			return false;
		}
	};
}
