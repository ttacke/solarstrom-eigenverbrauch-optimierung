#pragma once
#include "config.h"

namespace Local {
	class Wetter {
	protected:
		// Hinweis: index0 = je nach Startzeitpunkt. Das muss außerhalb noch behandelt werden
		// In einem extra Objekt? Irgendwo, bevor es hier landet??
		// TODO hier soll index0 immer der aktuelle Tage und die aktuelle Stunde sein. Nichts anderes!
		int stundenvorhersage_solarstrahlung_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		int tagesvorhersage_solarstrahlung_liste[5] = {0,0,0,0,0};

	public:
		int stundenvorhersage_startzeitpunkt;
		int tagesvorhersage_startzeitpunkt;

		void setze_stundenvorhersage_solarstrahlung(int index, int val) {
			stundenvorhersage_solarstrahlung_liste[index] = val;
		}

		int gib_stundenvorhersage_solarstrahlung_in_prozent(int index, Local::Config& cfg) {
			return round(stundenvorhersage_solarstrahlung_liste[index] * 100 / cfg.maximale_solarstrahlung_in_w_pro_m2);
		}

		void setze_tagesvorhersage_solarstrahlung(int index, int val) {
			tagesvorhersage_solarstrahlung_liste[index] = val;
		}

		int gib_tagesvorhersage_solarstrahlung_in_prozent(int index, Local::Config& cfg) {
			return round(tagesvorhersage_solarstrahlung_liste[index] * 100 / cfg.maximale_solarstrahlung_pro_tag_in_w_pro_m2);
		}

		void set_log_data(char* buffer) {
			sprintf(
				buffer,
				"whv1,%d,%d",
				stundenvorhersage_solarstrahlung_liste[0],
				tagesvorhersage_solarstrahlung_liste[0]
			);
		}
	};
}
