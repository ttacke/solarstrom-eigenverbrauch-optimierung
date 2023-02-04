#pragma once
#include "base_leser.h"
#include "persistenz.h"
#include "wetter.h"

namespace Local {
	class WettervorhersageLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	protected:
		const char* hourly_filename = "wetter_stundenvorhersage.json";
		const char* dayly_filename = "wetter_tagesvorhersage.json";
		const char* hourly_request_uri = "/forecasts/v1/hourly/12hour/%d?apikey=%s&language=de-de&details=true&metric=true";
		const char* dayly_request_uri = "/forecasts/v1/daily/5day/%d?apikey=%s&language=de-de&details=true&metric=true";
		char request_uri[128];

		void _daten_holen_und_persistieren(Local::Persistenz& persistenz, const char* filename, const char* uri) {
			// geht der Abruf schief, wird die vorherige Datei zerstoehrt.
			// Der entstehende Schaden ist nicht relevant genug, um sich darum zu kuemmern
			if(persistenz.open_file_to_overwrite(filename)) {
				sprintf(request_uri, uri, cfg->accuweather_location_id, cfg->accuweather_api_key);
				web_client->send_http_get_request(
					"dataservice.accuweather.com",
					80,
					request_uri
				);
				while(web_client->read_next_block_to_buffer()) {
					memcpy(persistenz.buffer, web_client->buffer, strlen(web_client->buffer) + 1);
					persistenz.print_buffer_to_file();
				}
				persistenz.close_file();
				Serial.println("Wetterdaten geschrieben");
			}
		}
	public:
		void stundendaten_holen_und_persistieren(Local::Persistenz& persistenz) {
			_daten_holen_und_persistieren(persistenz, hourly_filename, hourly_request_uri);
		}

		// TODO benutzen
		void tagesdaten_holen_und_persistieren(Local::Persistenz& persistenz) {
			_daten_holen_und_persistieren(persistenz, dayly_filename, dayly_request_uri);
		}

		void persistierte_daten_einsetzen(Local::Persistenz& persistenz, Local::Wetter& wetter) {
			int zeitpunkt_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			int solarstrahlung_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			int wolkendichte_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			int i = 0;
			if(persistenz.open_file_to_read(hourly_filename)) {
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
				wetter.stundenvorhersage_vorhanden = true;
				for(int i = 0; i < 12; i++) {
					wetter.setze_stundenvorhersage_solarstrahlung(i, solarstrahlung_liste[i]);
					wetter.setze_stundenvorhersage_wolkendichte(i, wolkendichte_liste[i]);
				}
			} else {
				wetter.stundenvorhersage_vorhanden = false;
			}
		}
	};
}
