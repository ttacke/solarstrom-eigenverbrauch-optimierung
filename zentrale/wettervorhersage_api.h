#pragma once
#include "base_api.h"
#include "persistenz.h"
#include "wetter.h"
#include <TimeLib.h>

namespace Local {
	class WettervorhersageAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		const char* hourly_filename = "wetter_stundenvorhersage.json";
		const char* hourly_cache_filename = "wetter_stundenvorhersage.csv";
		const char* dayly_filename = "wetter_tagesvorhersage.json";
		const char* dayly_cache_filename = "wetter_tagesvorhersage.csv";
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
			while(persistenz.read_next_block_to_buffer() && i < stunden_anzahl) {
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

		int _timestamp_to_date(int timestamp) {
			return day(timestamp) + (month(timestamp) * 100) + (year(timestamp) * 10000);
		}

		void _lese_tagesdaten_und_setze_ein(Local::Persistenz& persistenz, int now_timestamp) {
			if(!persistenz.open_file_to_read(dayly_filename)) {
				return;
			}

			int i = -1;
			std::uint8_t findings = 0b0000'0011;
			int letzte_gefundene_zeit = 0;
			int now_date = _timestamp_to_date(now_timestamp);
			while(persistenz.read_next_block_to_buffer() && i < tage_anzahl) {
				if(persistenz.find_in_content((char*) "\"EpochDate\":([0-9]+)[,}]")) {
					int zeitpunkt = atoi(persistenz.finding_buffer);
					if(
						letzte_gefundene_zeit != zeitpunkt // nur 1x behandeln
						&& _timestamp_to_date(zeitpunkt) >= now_date // Zu altes ueberspringen
					) {
						letzte_gefundene_zeit = zeitpunkt;
						i++;
						if(// das bezieht sich immer auf den Cache
							zeitpunkt_tage_liste[i] == 0
							||
							zeitpunkt_tage_liste[i] == zeitpunkt
						) {
							zeitpunkt_tage_liste[i] = zeitpunkt;
							solarstrahlung_tage_liste[i] = 0;
							findings = 0b0000'0000;
						}
					}
				}
				// Nur den Ganzzahlwert, Nachkommastellen sind irrelevant
				if(
					!(findings & 0b0000'0001)
					&&
					persistenz.find_in_content((char*) "\"SolarIrradiance\":{[^}]*\"Value\":([0-9.]+)[,}]")
				) {
					// Tag & Nacht (das sind 2 gleich lautende Keys)
					solarstrahlung_tage_liste[i] += round(atof(persistenz.finding_buffer));
					findings |= 0b0000'0001;
				}
				if(persistenz.find_in_content((char*) "\"Night\":")) {
					// Erst wenn der Nacht-Uebergang gefunden wurde, nochmal auslesen erlauben
					findings = 0b0000'0000;
				}
			}
			persistenz.close_file();
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

		void _lese_tagescache_und_setze_ein(Local::Persistenz& persistenz, int now_timestamp) {
			if(!persistenz.open_file_to_read(dayly_cache_filename)) {
				return;
			}
			int i = 0;
			int now_date = _timestamp_to_date(now_timestamp);
			while(persistenz.read_next_line_to_buffer() && i < tage_anzahl) {
				if(persistenz.find_in_content((char*) "^([0-9]+),([0-9]+)$")) {
					int cache_zeit = atoi(persistenz.finding_buffer);
					if(_timestamp_to_date(cache_zeit) < now_date) {
						continue;// Zu alt, ueberspringen
					}
					zeitpunkt_tage_liste[i] = cache_zeit;
					persistenz.fetch_next_finding();
					solarstrahlung_tage_liste[i] = atoi(persistenz.finding_buffer);
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
				sprintf(persistenz.buffer, "%d,%d\n", zeitpunkt_stunden_liste[i], solarstrahlung_stunden_liste[i]);
				persistenz.print_buffer_to_file();
			}
			persistenz.close_file();
		}

		void _schreibe_tagescache(Local::Persistenz& persistenz) {
			if(!persistenz.open_file_to_overwrite(dayly_cache_filename)) {
				return;
			}
			for(int i = 0; i < tage_anzahl; i++) {
				sprintf(persistenz.buffer, "%d,%d\n", zeitpunkt_tage_liste[i], solarstrahlung_tage_liste[i]);
				persistenz.print_buffer_to_file();
			}
			persistenz.close_file();
		}

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

			_reset(zeitpunkt_tage_liste, tage_anzahl);
			_reset(solarstrahlung_tage_liste, tage_anzahl);
			wetter.tagesvorhersage_startzeitpunkt = 0;
			for(int i = 0; i < tage_anzahl; i++) {
				wetter.setze_tagesvorhersage_solarstrahlung(i, 0);
			}

			_lese_tagescache_und_setze_ein(persistenz, now_timestamp);
			_lese_tagesdaten_und_setze_ein(persistenz, now_timestamp);
			wetter.tagesvorhersage_startzeitpunkt = zeitpunkt_tage_liste[0];
			for(int i = 0; i < tage_anzahl; i++) {
				wetter.setze_tagesvorhersage_solarstrahlung(i, solarstrahlung_tage_liste[i]);
			}
			_schreibe_tagescache(persistenz);
		}
	};
}
