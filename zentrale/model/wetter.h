#pragma once
#include "../config.h"

namespace Local::Model {
	class Wetter {
	protected:
		// Hinweis: index0 = ist immer "jetzt" bei Stunde und Tag
		// Stunde: gilt von 30min vor bis 30min nach
		int tagesvorhersage_solarstrahlung_liste[5] = {0,0,0,0,0};

	public:
		int stundenvorhersage_solarstrahlung_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		int stundenvorhersage_startzeitpunkt;
		int tagesvorhersage_startzeitpunkt;
		int zeitpunkt_sonnenuntergang = 0;

		void setze_stundenvorhersage_solarstrahlung(int index, int val) {
			stundenvorhersage_solarstrahlung_liste[index] = val;
		}

		void setze_tagesvorhersage_solarstrahlung(int index, int val) {
			tagesvorhersage_solarstrahlung_liste[index] = val;
		}

		int gib_tagesvorhersage_solarstrahlung_als_fibonacci(int index) {
			int strahlung = tagesvorhersage_solarstrahlung_liste[index];
			int a = 80;
			int b = 80;
			int tmp;
			for(int i = 0; i < 10; i++) {
				if(strahlung <= b) {
					return i * 10;
				}
				tmp = b;
				b = a + b;
				a = tmp;
			}
			return 100;
		}

		void write_log_data(Local::Service::FileWriter& file_writer) {
			file_writer.write_formated(
				"w2,%d,%d,%d",
				stundenvorhersage_solarstrahlung_liste[0],
				tagesvorhersage_solarstrahlung_liste[0],
				zeitpunkt_sonnenuntergang
			);
		}
	};
}
