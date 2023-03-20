#pragma once
#include <SPI.h>
#include <SD.h>

namespace Local::Service {
	class FileWriter {
	protected:
		bool init = false;
		File fh;
		char buffer[2048];
		int buffer_offset = 0;

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

	public:
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
			fh = SD.open(filename, FILE_WRITE);
			std::fill(buffer, buffer + sizeof(buffer), 0);
			buffer_offset = 0;
			return true;
		}

		void write(char* string) {
			int string_length = strlen(string);
			int buffer_space_length = sizeof(buffer) - buffer_offset - 1;
			int read_string_till = std::min(string_length, buffer_space_length);
			memcpy(buffer + buffer_offset, string, read_string_till);
			if(read_string_till < string_length) {
				fh.print(buffer);
				buffer_offset = 0;
				string_length = string_length - read_string_till;
				memcpy(buffer + buffer_offset, string + read_string_till, string_length);
				std::fill(buffer + buffer_offset + string_length + 1, buffer + sizeof(buffer), 0);
			}
			buffer_offset += string_length;
		}

		void close_file() {
			fh.print(buffer);
			fh.close();
			std::fill(buffer, buffer + sizeof(buffer), 0);
			buffer_offset = 0;
		}

		bool delete_file(const char* filename) {
			if(!_init() || !SD.exists(filename)) {
				return false;
			}
			return SD.remove(filename);
		}
	};
}
