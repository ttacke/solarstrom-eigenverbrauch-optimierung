#pragma once
#include "base_leser.h"
#include "persistenz.h"
#include "wetter.h"

namespace Local {
	class WettervorhersageLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	protected:
		const char* filename = "wetter_stundenvorhersage.json";

	public:
		void daten_holen_und_persistieren(Local::Persistenz& persistenz) {
			// TODO geht der Abruf schief, wird die vorherige Datei zerstoehrt. Das muss anders sein
			if(persistenz.open_file_to_overwrite(filename)) {
				web_client->send_http_get_request(
					"dataservice.accuweather.com",
					80,
					cfg->wetter_stundenvorhersage_request_url
				);
				while(web_client->read_next_block_to_buffer()) {
					memcpy(persistenz.buffer, web_client->buffer, strlen(web_client->buffer) + 1);
					persistenz.print_buffer_to_file();
				}
				persistenz.close_file();
				Serial.println("Wetterdaten geschrieben");
			}
		}

		void persistierte_daten_einsetzen(Local::Persistenz& persistenz, Local::Wetter& wetter) {
			int zeitpunkt_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			int solarstrahlung_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			int wolkendichte_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			int i = 0;
			if(persistenz.open_file_to_read(filename)) {
				while(persistenz.read_next_block_to_buffer()) {
					if(persistenz.find_in_content((char*) "\"EpochDateTime\":([0-9]+)[,}]")) {
						int zeitpunkt = atoi(persistenz.finding_buffer);
						if(zeitpunkt != 0 && zeitpunkt_liste[i] != zeitpunkt) {
							if(zeitpunkt_liste[0] != 0) {
								i++;
							}
							zeitpunkt_liste[i] = zeitpunkt;
						}
					}
					// Nur den Ganzzahlwert, Nachkommastellen sind irrelevant
					if(persistenz.find_in_content((char*) "\"SolarIrradiance\":{[^}]*\"Value\":([0-9.]+)[,}]")) {
						int solarstrahlung = round(atof(persistenz.finding_buffer));
						if(solarstrahlung != 0 && solarstrahlung_liste[i] != solarstrahlung) {
							solarstrahlung_liste[i] = solarstrahlung;
						}
					}

					if(persistenz.find_in_content((char*) "\"CloudCover\":([0-9]+)[,}]")) {
						int wolkendichte = round(atof(persistenz.finding_buffer));
						if(wolkendichte != 0 && wolkendichte_liste[i] != wolkendichte) {
							wolkendichte_liste[i] = wolkendichte;
						}
					}
				}
				persistenz.close_file();
			}
			if(zeitpunkt_liste[0] > 0) {
				wetter.daten_vorhanden = true;
				for(int i = 0; i < 12; i++) {
					wetter.setze_stundenvorhersage_solarstrahlung(i, solarstrahlung_liste[i]);
					wetter.setze_stundenvorhersage_wolkendichte(i, wolkendichte_liste[i]);
				}
			} else {
				wetter.daten_vorhanden = false;
			}
		}
	};
}
