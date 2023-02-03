#pragma once
#include <SPI.h>
#include <SD.h>

namespace Local {
	class Persistenz {
	protected:
		bool init = false;
		File fh;
		MatchState match_state;
		int offset = 0;
		char old_buffer[64];
		uint8_t bin_buffer[63];
		char search_buffer[128];

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
			int old_buffer_strlen = strlen(old_buffer);
			memcpy(search_buffer, old_buffer, old_buffer_strlen + 1);
			//TODO memcpy(&localData, incomingPacket + 6, sizeof(localData));
			// mit + kann man auch den ZielPointer verschieben. Das schreibt direkt in dem RAM
			for(int i = 0; i < strlen(buffer) + 1; i++) {
				search_buffer[old_buffer_strlen + i] = buffer[i];
			}
		}

	public:
		char finding_buffer[65];
		char buffer[64];

		void close_file() {
			fh.close();
		}

		bool open_file_to_read(const char* filename) {
			if(!_init() || !SD.exists(filename)) {
				return false;
			}
			offset = 0;
			buffer[0] = '\0';
			fh = SD.open(filename);
			return true;
		}

		bool read_next_block_to_buffer() {
			int size = fh.size();
			fh.seek(offset);
			int read_size = std::min(sizeof(bin_buffer) - 1, fh.size() - offset);
			if(read_size == 0) {
				return false;
			}

			memcpy(old_buffer, buffer, strlen(buffer) + 1);
			fh.read(bin_buffer, read_size);
			for (int i = 0; i < read_size; i++) {
				buffer[i] = (char) bin_buffer[i];
			}
			buffer[read_size + 1] = '\0';
			_prepare_search_buffer();

			offset += read_size;
			return true;
		}

		bool find_in_content(char* regex) {
			match_state.Target(search_buffer);
			char result = match_state.Match(regex);
			if(result > 0) {
				match_state.GetCapture(finding_buffer, 0);
				return true;
			}
			return false;
		}

		bool open_file_to_overwrite(const char* filename) {
			if(!_init()) {
				return false;
			}
			fh = SD.open(filename, T_CREATE | O_WRITE | O_TRUNC);
			fh.close();

			return open_file_to_append(filename);
		}

		bool open_file_to_append(const char* filename) {
			if(!_init()) {
				return false;
			}
			offset = 0;
			buffer[0] = '\0';
			fh = SD.open(filename, FILE_WRITE);
			return true;
		}

		void print_buffer_to_file() {
			fh.print(buffer);
		}
	};
}
