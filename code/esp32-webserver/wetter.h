#pragma once

namespace Local {
	class Wetter {
	public:
		bool daten_vorhanden = false;
		int stundenvorhersage_solarstrahlung_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		int stundenvorhersage_wolkendichte_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

		void setze_stundenvorhersage_solarstrahlung(int stunden_in_der_zukunft, int solarstrahlung) {
			stundenvorhersage_solarstrahlung_liste[stunden_in_der_zukunft] = solarstrahlung;
		}

		int gib_stundenvorhersage_solarstrahlung(int stunden_in_der_zukunft) {
			return stundenvorhersage_solarstrahlung_liste[stunden_in_der_zukunft];
		}

		void setze_stundenvorhersage_wolkendichte(int stunden_in_der_zukunft, int wolkendichte) {
			stundenvorhersage_wolkendichte_liste[stunden_in_der_zukunft] = wolkendichte;
		}

		int gib_stundenvorhersage_wolkendichte(int stunden_in_der_zukunft) {
			return stundenvorhersage_wolkendichte_liste[stunden_in_der_zukunft];
		}

		void set_log_data(char* buffer) {
			sprintf(
				buffer,
				"whv1;%d,%d,%d,%d",
				stundenvorhersage_solarstrahlung_liste[0],
				stundenvorhersage_wolkendichte_liste[0],
				(daten_vorhanden ? 1 : 0)
			);
		}
	};
}
