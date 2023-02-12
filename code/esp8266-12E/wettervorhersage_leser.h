#pragma once
#include "base_leser.h"
#include "persistenz.h"
#include "wetter.h"

namespace Local {
	class WettervorhersageLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	protected:
		const char* hourly_filename = "wetter_stundenvorhersage.json";
		const char* hourly_cache_filename = "wetter_stundenvorhersage.csv";
		const char* dayly_filename = "wetter_tagesvorhersage.json";
		const char* hourly_request_uri = "/forecasts/v1/hourly/12hour/%d?apikey=%s&language=de-de&details=true&metric=true";
		const char* dayly_request_uri = "/forecasts/v1/daily/5day/%d?apikey=%s&language=de-de&details=true&metric=true";
		char request_uri[128];

		int zeitpunkt_tage_liste[5];
		int solarstrahlung_tage_liste[5];
		int tage_anzahl = 5;
		int zeitpunkt_stunden_liste[12];
		int solarstrahlung_stunden_liste[12];
		int stunden_anzahl = 12;// warum ist sizeof(solarstrahlung_stunden_liste) hier ein OOM Fatal? Ist das doch ein pointer, weil ein array?

		void _reset(int* liste, size_t length) {
			for(int i = 0; i < length; i++) {
				liste[i] = 0;
			}
		}

		void _daten_holen_und_persistieren(Local::Persistenz& persistenz, const char* filename, const char* uri) {
			// geht der Abruf schief, wird die vorherige Datei zerstoehrt.
			// Der entstehende Schaden ist nicht relevant genug, um sich darum zu kuemmern
			if(!persistenz.open_file_to_overwrite(filename)) {
				return;
			}
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

		void _lese_stundendaten_und_setze_ein(Local::Persistenz& persistenz, int now_timestamp) {
			if(!persistenz.open_file_to_read(hourly_filename)) {
				return;
			}

			int i = -1;
			std::uint8_t findings = 0b0000'0001;
			int letzte_gefundene_zeit = 0;
			while(persistenz.read_next_block_to_buffer()) {
				if(persistenz.find_in_content((char*) "\"EpochDateTime\":([0-9]+)[,}]")) {
					int zeitpunkt = atoi(persistenz.finding_buffer);
					if(
						letzte_gefundene_zeit != zeitpunkt // nur 1x behandeln
						&& now_timestamp < zeitpunkt + 1800 // Zu altes ueberspringen
					) {
						letzte_gefundene_zeit = zeitpunkt;
						i++;
						if(// das bezieht sich immer auf den Cache
							zeitpunkt_stunden_liste[i] == 0
							||
							zeitpunkt_stunden_liste[i] == zeitpunkt
						) {
							zeitpunkt_stunden_liste[i] = zeitpunkt;
							findings = 0b0000'0000;
						}
					}
				}

				if(
					!(findings & 0b000'0001) // Nur das erste passend zur gefundenen Zeit ermitteln
					// Nur den Ganzzahlwert, Nachkommastellen sind irrelevant
					&& persistenz.find_in_content((char*) "\"SolarIrradiance\":{[^}]*\"Value\":([0-9.]+)[,}]")
				) {
					solarstrahlung_stunden_liste[i] = round(atof(persistenz.finding_buffer));
					findings = 0b0000'0001;
				}
			}
			persistenz.close_file();
		}

		bool _lese_tagesdaten_und_setze_ein(Local::Persistenz& persistenz) {
			_reset(zeitpunkt_tage_liste, tage_anzahl);
			_reset(solarstrahlung_tage_liste, tage_anzahl);
			if(!persistenz.open_file_to_read(dayly_filename)) {
				return false;
			}

			std::uint8_t findings = 0b0000'0000;
			int i = 0;
			int valide_tage = 0;
			while(persistenz.read_next_block_to_buffer()) {
				if(persistenz.find_in_content((char*) "\"EpochDate\":([0-9]+)[,}]")) {
					int zeitpunkt = atoi(persistenz.finding_buffer);
					if(zeitpunkt_tage_liste[i] != zeitpunkt) {
						if(zeitpunkt_tage_liste[0] != 0) {
							i++;
							if(findings & 0b000'0011) {
								valide_tage++;
							}
							findings = 0b0000'0000;
						}
						zeitpunkt_tage_liste[i] = zeitpunkt;
					}
				}
				// Nur den Ganzzahlwert, Nachkommastellen sind irrelevant
				if(persistenz.find_in_content((char*) "\"SolarIrradiance\":{[^}]*\"Value\":([0-9.]+)[,}]")) {
					int solarstrahlung = round(atof(persistenz.finding_buffer));
					if(!(findings & 0b0000'0001)) { // Tag ...
						solarstrahlung_tage_liste[i] += solarstrahlung;
						findings |= 0b0000'0001;
					} else if(!(findings & 0b0000'0010)) { // ...und Nacht werden addiert
						solarstrahlung_tage_liste[i] += solarstrahlung;
						findings |= 0b0000'0010;
					}
				}
			}
			persistenz.close_file();

			if(findings & 0b0000'0011) {
				valide_tage++;
			}
			if(valide_tage == 5) {
				return true;
			}
			_reset(zeitpunkt_tage_liste, tage_anzahl);
			_reset(solarstrahlung_tage_liste, tage_anzahl);
			return false;
		}

		void _lese_stundencache_und_setze_ein(Local::Persistenz& persistenz, int now_timestamp) {
			if(!persistenz.open_file_to_read(hourly_cache_filename)) {
				return;
			}
			int i = 0;
			while(persistenz.read_next_line_to_buffer() && i < stunden_anzahl) {
				if(persistenz.find_in_content((char*) "^([0-9]+),([0-9]+)$")) {
					int cache_zeit = atoi(persistenz.finding_buffer);
					if(now_timestamp - 1800 > cache_zeit) {
						continue;// Zu alt, ueberspringen
					}
					zeitpunkt_stunden_liste[i] = cache_zeit;
					persistenz.fetch_next_finding();
					solarstrahlung_stunden_liste[i] = atoi(persistenz.finding_buffer);
					i++;
				}
			}
			persistenz.close_file();
		}

		void _schreibe_stundencache(Local::Persistenz& persistenz) {
			if(!persistenz.open_file_to_overwrite(hourly_cache_filename)) {
				return;
			}
			for(int i = 0; i < stunden_anzahl; i++) {
				sprintf(persistenz.buffer, "%d,%d\n", zeitpunkt_stunden_liste[0], solarstrahlung_stunden_liste[i]);
				persistenz.print_buffer_to_file();
			}
			persistenz.close_file();
		}

	// TODO in indx.html setTieout absichern. Das übreholt sich selber
	// TODO configwerte als SD_File ablegen - und auch wie CURL schreibbar machen -> dan können komlpexere Dinge außerhalb laufen
	// TODO das beim Status-File Lesen auch nutzen
	// TODO in UI: zeit == 0 dann kein Label erstellen! (bei den Wetterdaten)
	public:
		void stundendaten_holen_und_persistieren(Local::Persistenz& persistenz) {
			_daten_holen_und_persistieren(persistenz, hourly_filename, hourly_request_uri);
		}

		void tagesdaten_holen_und_persistieren(Local::Persistenz& persistenz) {
			_daten_holen_und_persistieren(persistenz, dayly_filename, dayly_request_uri);
		}

		void persistierte_daten_einsetzen(
			Local::Persistenz& persistenz, Local::Wetter& wetter, int now_timestamp
		) {
			_reset(zeitpunkt_stunden_liste, stunden_anzahl);
			_reset(solarstrahlung_stunden_liste, stunden_anzahl);
			wetter.stundenvorhersage_startzeitpunkt = 0;
			for(int i = 0; i < stunden_anzahl; i++) {
				wetter.setze_stundenvorhersage_solarstrahlung(i, 0);
			}

			_lese_stundencache_und_setze_ein(persistenz, now_timestamp);
			_lese_stundendaten_und_setze_ein(persistenz, now_timestamp);
			wetter.stundenvorhersage_startzeitpunkt = zeitpunkt_stunden_liste[0];
			for(int i = 0; i < stunden_anzahl; i++) {
				wetter.setze_stundenvorhersage_solarstrahlung(i, solarstrahlung_stunden_liste[i]);
			}
			_schreibe_stundencache(persistenz);

			// TODO auch cache incl korrektur einfuegen
			if(_lese_tagesdaten_und_setze_ein(persistenz)) {
				wetter.tagesvorhersage_startzeitpunkt = zeitpunkt_tage_liste[0];
				for(int i = 0; i < 5; i++) {
					wetter.setze_tagesvorhersage_solarstrahlung(i, solarstrahlung_tage_liste[i]);
				}
			} else {
				wetter.tagesvorhersage_startzeitpunkt = 0;
				for(int i = 0; i < 5; i++) {
					wetter.setze_tagesvorhersage_solarstrahlung(i, 0);
				}
			}
		}
	};
}
