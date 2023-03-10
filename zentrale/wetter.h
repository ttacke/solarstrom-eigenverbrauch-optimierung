#pragma once
#include "config.h"

namespace Local {
	class Wetter {
	protected:
		// Hinweis: index0 = ist immer "jetzt" bei Stunde und Tag
		// Stunde: gilt von 30min vor bis 30min nach
		int stundenvorhersage_solarstrahlung_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		int tagesvorhersage_solarstrahlung_liste[5] = {0,0,0,0,0};

	public:
		int stundenvorhersage_startzeitpunkt;
		int tagesvorhersage_startzeitpunkt;
		int zeitpunkt_sonnenuntergang = 0;

		void setze_stundenvorhersage_solarstrahlung(int index, int val) {
			stundenvorhersage_solarstrahlung_liste[index] = val;
		}

		int gib_stundenvorhersage_solarstrahlung_in_prozent(int index, Local::Config& cfg) {
			return round((float) stundenvorhersage_solarstrahlung_liste[index] * 100 / (float) cfg.maximale_solarstrahlung_in_w_pro_m2);
		}

		void setze_tagesvorhersage_solarstrahlung(int index, int val) {
			tagesvorhersage_solarstrahlung_liste[index] = val;
		}

		int gib_tagesvorhersage_solarstrahlung_in_prozent(int index, Local::Config& cfg) {
			return round((float) tagesvorhersage_solarstrahlung_liste[index] * 100 / (float) cfg.maximale_solarstrahlung_pro_tag_in_w_pro_m2);
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
