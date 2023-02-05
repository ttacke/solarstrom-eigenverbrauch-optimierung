#pragma once

namespace Local {
	class Wetter {
	protected:
		// index0 = die naechste Stunde. "Jetzt" ist nicht enthalten
		// TODO nicht ganz! Die Daten kommen vermutlich irgendwann, der Zeitpunkt der Daten muss ausgewertet werden
		int stundenvorhersage_solarstrahlung_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		int stundenvorhersage_wolkendichte_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		// TODO index0 = ???, das kann sogarGEstern sein (1:20 war der Stand: 7 Uhr am Vortag)
		// TODO D.h. 1x am Tag reicht, gegen 8 Uhr
		int tagesvorhersage_solarstrahlung_liste[5] = {0,0,0,0,0};

	public:
		bool stundenvorhersage_vorhanden = false;

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
				"whv1;%d,%d,%d,%d",
				stundenvorhersage_solarstrahlung_liste[0],
				stundenvorhersage_wolkendichte_liste[0],
				(stundenvorhersage_vorhanden ? 1 : 0),
				tagesvorhersage_solarstrahlung_liste[0]
			);
		}
	};
}
