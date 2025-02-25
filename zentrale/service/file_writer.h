#pragma once
#include <SPI.h>
#include <SD.h>

namespace Local::Service {
	class FileWriter {
	protected:
		bool init = false;
		File fh;
		char buffer[2048];
		char format_buffer[64];
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
			if(!delete_file(filename)) {
				return false;
			}
			if(!_init()) {
				return false;
			}
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

		void write_formated(const char* format, ...) {
			va_list arglist;
			va_start(arglist, format);
			vsprintf(format_buffer, format, arglist);
			va_end(arglist);

			write((uint8_t*) format_buffer, strlen(format_buffer));
		}

		void write(uint8_t* string, int string_length) {
			write((char*) string, string_length);
		}

		void write(char* string, int string_length) {
			int buffer_space_length = sizeof(buffer) - buffer_offset - 1;
			int read_string_till = std::min(string_length, buffer_space_length);
			memcpy(buffer + buffer_offset, string, read_string_till);
			if(read_string_till < string_length) {
				fh.print(buffer);
				std::fill(buffer, buffer + sizeof(buffer), 0);
				buffer_offset = 0;
				return write(string + read_string_till, string_length - read_string_till);
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
			if(!_init()) {
				return false;
			}
			if(SD.exists(filename) != 1 || SD.remove(filename) == 1) {
				return true;
			}
			// DEBUG: Speicherfehler?
			Serial.println("Cant delete file:");
			Serial.println(filename);
			return false;
		}
	};
}
