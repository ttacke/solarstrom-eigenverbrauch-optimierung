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

		void _lese_stundendaten_und_setze_ein(Local::Persistenz& persistenz, Local::Wetter& wetter) {
			wetter.stundenvorhersage_startzeitpunkt = 0;
			for(int i = 0; i < 12; i++) {
				wetter.setze_stundenvorhersage_solarstrahlung(i, 0);
			}
			if(!persistenz.open_file_to_read(hourly_filename)) {
				return;
			}
			int zeitpunkt_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			int solarstrahlung_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			std::uint8_t findings = 0b0000'0000;
			int i = 0;
			int valide_stunden = 0;
			while(persistenz.read_next_block_to_buffer()) {
				if(persistenz.find_in_content((char*) "\"EpochDateTime\":([0-9]+)[,}]")) {
					int zeitpunkt = atoi(persistenz.finding_buffer);
					if(zeitpunkt_liste[i] != zeitpunkt) {
						if(zeitpunkt_liste[0] != 0) {
							i++;
							if(findings & 0b0000'0001) {
								valide_stunden++;
							}
							findings = 0b0000'0000;
						}
						zeitpunkt_liste[i] = zeitpunkt;
					}
				}
				// Nur den Ganzzahlwert, Nachkommastellen sind irrelevant
				if(persistenz.find_in_content((char*) "\"SolarIrradiance\":{[^}]*\"Value\":([0-9.]+)[,}]")) {
					solarstrahlung_liste[i] = round(atof(persistenz.finding_buffer));
					findings |= 0b0000'0001;
				}
			}
			persistenz.close_file();

			if(findings & 0b0000'0001) {
				valide_stunden++;
			}
			if(valide_stunden == 12 && zeitpunkt_liste[0] > 0) {
				wetter.stundenvorhersage_startzeitpunkt = zeitpunkt_liste[0];
				for(int i = 0; i < 12; i++) {
					wetter.setze_stundenvorhersage_solarstrahlung(i, solarstrahlung_liste[i]);
				}
			} else {
				wetter.stundenvorhersage_startzeitpunkt = 0;
				for(int i = 0; i < 12; i++) {
					wetter.setze_stundenvorhersage_solarstrahlung(i, 0);
				}
			}
		}

		void _lese_tagesdaten_und_setze_ein(Local::Persistenz& persistenz, Local::Wetter& wetter) {
			wetter.tagesvorhersage_startzeitpunkt = 0;
			for(int i = 0; i < 5; i++) {
				wetter.setze_tagesvorhersage_solarstrahlung(i, 0);
			}
			if(!persistenz.open_file_to_read(dayly_filename)) {
				return;
			}
			int zeitpunkt_liste[5] = {0,0,0,0,0};
			int solarstrahlung_liste[5] = {0,0,0,0,0};
			std::uint8_t findings = 0b0000'0000;
			int i = 0;
			int valide_tage = 0;
			while(persistenz.read_next_block_to_buffer()) {
				if(persistenz.find_in_content((char*) "\"EpochDate\":([0-9]+)[,}]")) {
					int zeitpunkt = atoi(persistenz.finding_buffer);
					if(zeitpunkt_liste[i] != zeitpunkt) {
						if(zeitpunkt_liste[0] != 0) {
							i++;
							if(findings & 0b000'0011) {
								valide_tage++;
							}
							findings = 0b0000'0000;
						}
						zeitpunkt_liste[i] = zeitpunkt;
					}
				}
				// Nur den Ganzzahlwert, Nachkommastellen sind irrelevant
				if(persistenz.find_in_content((char*) "\"SolarIrradiance\":{[^}]*\"Value\":([0-9.]+)[,}]")) {
					int solarstrahlung = round(atof(persistenz.finding_buffer));
					if(!(findings & 0b0000'0001)) { // Tag ...
						solarstrahlung_liste[i] += solarstrahlung;
						findings |= 0b0000'0001;
					} else if(!(findings & 0b0000'0010)) { // ...und Nacht werden addiert
						solarstrahlung_liste[i] += solarstrahlung;
						findings |= 0b0000'0010;
					}
				}
			}
			persistenz.close_file();

			if(findings & 0b0000'0011) {
				valide_tage++;
			}
			if(valide_tage == 5) {
				wetter.tagesvorhersage_startzeitpunkt = zeitpunkt_liste[0];
				for(int i = 0; i < 5; i++) {
					wetter.setze_tagesvorhersage_solarstrahlung(i, solarstrahlung_liste[i]);
				}
			} else {
				wetter.tagesvorhersage_startzeitpunkt = 0;
				for(int i = 0; i < 5; i++) {
					wetter.setze_tagesvorhersage_solarstrahlung(i, 0);
				}
			}
		}

	public:
		void stundendaten_holen_und_persistieren(Local::Persistenz& persistenz) {
			_daten_holen_und_persistieren(persistenz, hourly_filename, hourly_request_uri);
		}

		void tagesdaten_holen_und_persistieren(Local::Persistenz& persistenz) {
			_daten_holen_und_persistieren(persistenz, dayly_filename, dayly_request_uri);
		}

		void persistierte_daten_einsetzen(Local::Persistenz& persistenz, Local::Wetter& wetter) {
			_lese_stundendaten_und_setze_ein(persistenz, wetter);
			_lese_tagesdaten_und_setze_ein(persistenz, wetter);
		}
	};
}
