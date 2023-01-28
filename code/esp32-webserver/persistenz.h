#pragma once
#include <SPI.h>
#include <SD.h>

namespace Local {
	class Persistenz {
	protected:
		bool init = false;
		File fh;
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
		void append2file(char* filename, String content) {
			if(!_init()) {
				return;
			}
			File dataFile = SD.open(filename, FILE_WRITE);
			dataFile.print(content);
			dataFile.close();
		}
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
