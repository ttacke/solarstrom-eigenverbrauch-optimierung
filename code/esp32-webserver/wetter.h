#pragma once

namespace Local {
	class Wetter {
	protected:
		// index0 = je nach Startzeitpunkt, meist naechste Stunde
		int stundenvorhersage_solarstrahlung_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		int stundenvorhersage_wolkendichte_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		// index0 = je nach Startzeitpunkt, kann auch ma "Gestern" sein
		int tagesvorhersage_solarstrahlung_liste[5] = {0,0,0,0,0};

	public:
		int stundenvorhersage_startzeitpunkt;
		bool stundenvorhersage_ist_valide = false;
		int tagesvorhersage_startzeitpunkt;
		bool tagesvorhersage_ist_valide = false;

		void setze_stundenvorhersage_solarstrahlung(int index, int solarstrahlung) {
			stundenvorhersage_solarstrahlung_liste[index] = solarstrahlung;
		}

		int gib_stundenvorhersage_solarstrahlung(int index) {
			return stundenvorhersage_solarstrahlung_liste[index];
		}

		void setze_stundenvorhersage_wolkendichte(int index, int wolkendichte) {
			stundenvorhersage_wolkendichte_liste[index] = wolkendichte;
		}

		int gib_stundenvorhersage_wolkendichte(int stunden_in_der_zukunft) {
			return stundenvorhersage_wolkendichte_liste[stunden_in_der_zukunft];
		}

		void setze_tagesvorhersage_solarstrahlung(int index, int solarstrahlung) {
			tagesvorhersage_solarstrahlung_liste[index] = solarstrahlung;
		}

		int gib_tagesvorhersage_solarstrahlung(int index) {
			return tagesvorhersage_solarstrahlung_liste[index];
		}

		void set_log_data(char* buffer) {
			sprintf(
				buffer,
				"whv1,%d,%d,%d,%d,%d,%d,%d",
				(stundenvorhersage_ist_valide ? 1 : 0),
				stundenvorhersage_startzeitpunkt,
				stundenvorhersage_solarstrahlung_liste[0],
				stundenvorhersage_wolkendichte_liste[0],
				(tagesvorhersage_ist_valide ? 1 : 0),
				tagesvorhersage_startzeitpunkt,
				tagesvorhersage_solarstrahlung_liste[0]
			);
		}
	};
}
