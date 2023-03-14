#pragma once
#include "config.h"

namespace Local {
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

// TODO DEPRECATED
//		int gib_stundenvorhersage_solarstrahlung_in_prozent(int index, Local::Config& cfg) {
//			return round((float) stundenvorhersage_solarstrahlung_liste[index] * 100 / (float) cfg.maximale_solarstrahlung_in_w_pro_m2);
//		}

		void setze_tagesvorhersage_solarstrahlung(int index, int val) {
			tagesvorhersage_solarstrahlung_liste[index] = val;
		}

// TODO DEPRECATED
//		int gib_tagesvorhersage_solarstrahlung_in_prozent(int index, Local::Config& cfg) {
//			return round((float) tagesvorhersage_solarstrahlung_liste[index] * 100 / (float) cfg.maximale_solarstrahlung_pro_tag_in_w_pro_m2);
//		}

		int gib_tagesvorhersage_solarstrahlung_als_fibonacci(int index) {
			int strahlung = tagesvorhersage_solarstrahlung_liste[index];
			int a = 100;
			int b = 100;
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

		void set_log_data(char* buffer) {
			sprintf(
				buffer,
				"w2,%d,%d,%d",
				stundenvorhersage_solarstrahlung_liste[0],
				tagesvorhersage_solarstrahlung_liste[0],
				zeitpunkt_sonnenuntergang
			);
		}
	};
}
