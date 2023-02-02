#pragma once
#include <SPI.h>
#include <SD.h>

namespace Local {
	class Persistenz {
	protected:
		bool init = false;
		File fh;
		uint8_t bin_buffer[64];
		char old_buffer[64];
		char* buffer;
		char search_buffer[128];
		int offset = 0;

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
		bool open_file_to_read(const char* filename) {
			if(!_init() || !SD.exists(filename)) {
				return false;
			}
			offset = 0;
			fh = SD.open(filename);
			return true;
		}

		bool read_next_block_to_buffer() {
			int size = fh.size();
			fh.seek(offset);
			int read_size = std::min(sizeof(bin_buffer), fh.size() - offset);
			if(read_size == 0) {
				return false;
			}

			fh.read(bin_buffer, read_size);
			buffer = (char*) bin_buffer;
			offset += read_size;
			return true;
		}

//		void append2file(char* filename, String content) {
//			if(!_init()) {
//				return;
//			}
//			File dataFile = SD.open(filename, FILE_WRITE);
//			dataFile.print(content);
//			dataFile.close();
//		}

//		void write2file(char* filename, String content) {
//			if(!_init()) {
//				return;
//			}
//			File dataFile1 = SD.open(filename, T_CREATE | O_WRITE | O_TRUNC);
//			dataFile1.close();
//
//			File dataFile = SD.open(filename, FILE_WRITE);
//			//dataFile.seek(0);
//			dataFile.print(content);
//			dataFile.close();
//		}




//
//
//		String read_file_content_block(char* filename, int offset) {
//			if(!SD.exists(filename)) {
//				return (String) "";
//			}
//			File fh = SD.open(filename);
//			fh.seek(offset);
//			int size = 64;
//			if(fh.size() - offset < size) {
//				size = fh.size() - offset;
//			}
//			uint8_t buffer[size];
//			fh.read(buffer, size - 1);
//			buffer[size - 1] = 0;// Null an Ende setzen
//			char content[size];
//			for (int i = 0; i < size; i++) {
//				content[i] = (char) buffer[i];
//			}
//			return (String) content;
//		}

//		String read_file_content(char* filename) {
//			if(!SD.exists(filename)) {
//				return (String) "";
//			}
//			File fh = SD.open(filename);
//			int size = fh.size();
//			uint8_t buffer[size + 1];
//			fh.read(buffer, size);
//			buffer[size] = 0;// Null an Ende setzen
//			char content[size + 1];
//			for (int i = 0; i < size + 1; i++) {
//				content[i] = (char) buffer[i];
//			}
//			return (String) content;// TODO das macht eine Kopie. Geht das auch ohne?
//		}

//		read(char* filename) {
//			fh = SD.open(filename);
//			if (fh) {
//				while (fh.available()) {// TODO wie liest der? Wir sollen bereiche haben. Oder Blockweise.
//					Serial.write(fh.read());
//				}
//				fh.close();
//			}
//		}
	};
}
