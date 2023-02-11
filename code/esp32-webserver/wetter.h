#pragma once

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
		bool stundenvorhersage_ist_valide = false;
		int tagesvorhersage_startzeitpunkt;
		bool tagesvorhersage_ist_valide = false;

		void setze_stundenvorhersage_solarstrahlung(int index, int val) {
			stundenvorhersage_solarstrahlung_liste[index] = val;
		}

		int gib_stundenvorhersage_solarstrahlung(int index) {
			return stundenvorhersage_solarstrahlung_liste[index];
		}

		void setze_tagesvorhersage_solarstrahlung(int index, int val) {
			tagesvorhersage_solarstrahlung_liste[index] = val;
		}

		int gib_tagesvorhersage_solarstrahlung(int index) {
			return tagesvorhersage_solarstrahlung_liste[index];
		}

		void set_log_data(char* buffer) {
			sprintf(
				buffer,
				"whv1,%d,%d",
				(stundenvorhersage_ist_valide ? stundenvorhersage_solarstrahlung_liste[0] : -1),
				(tagesvorhersage_ist_valide ? tagesvorhersage_solarstrahlung_liste[0] : -1)
			);
		}
	};
}
