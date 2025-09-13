#pragma once
#include "base_api.h"
#include "../service/file_reader.h"
#include "../model/wetter.h"
#include <TimeLib.h>

namespace Local::Api {
	class WettervorhersageAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		// SD Daten korrumpieren innerhalb weniger Jahre, daher 1x im Jahr autom. wechseln
		char filename_buffer[40];
		const char* roof1_filename_template = "dach1_wettervorhersage_%04d.json";
		const char* roof2_filename_template = "dach2_wettervorhersage_%04d.json";
		// TODO DEPRECATED
		const char* hourly_filename_template = "wetter_stundenvorhersage_%04d.json";
		const char* hourly_cache_filename_template = "wetter_stundenvorhersage_%04d.csv";
		const char* dayly_filename_template = "wetter_tagesvorhersage_%04d.json";
		const char* dayly_cache_filename_template = "wetter_tagesvorhersage_%04d.csv";
		const char* request_uri = "/v1/forecast?latitude=%d&longitude=%d&daily=sunrise,sunset,shortwave_radiation_sum&hourly=global_tilted_irradiance_instant&timezone=Europe%2FBerlin&tilt=%d&azimuth=%d&timeformat=unixtime&forecast_hours=12";
		char request_uri_buffer[128];

		int zeitpunkt_sonnenuntergang = 0;
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

		void _daten_holen_und_persistieren(
			Local::Service::FileReader& file_reader,
			Local::Service::FileWriter& file_writer,
			const char* filename, int now_timestamp,
			const float neigung_in_grad, const float azimuth
		) {
			// geht der Abruf schief, wird die vorherige Datei zerstoehrt.
			// Der entstehende Schaden ist nicht relevant genug, um sich darum zu kuemmern
			if(!file_writer.open_file_to_overwrite(filename)) {
				Serial.println("Schreibfehler!");
				return;
			}
			sprintf(
				request_uri_buffer,
				request_uri,
				cfg->wettervorhersage_lat,
				cfg->wettervorhersage_lon,
				neigung_in_grad,
				azimuth
			);
			// TODO dach2: nur die Stundenwerte zusammenrechnen
			// Test#1: prüfen, ob die Daten korrekt geschrieben werden

			web_reader->send_http_get_request(
				"api.open-meteo.com",
				80,
				request_uri
			);
			while(web_reader->read_next_block_to_buffer()) {
				file_writer.write(web_reader->buffer, strlen(web_reader->buffer));
			}
			file_writer.close_file();
			Serial.println("Wetterdaten geschrieben");
		}

		void _lese_stundendaten_und_setze_ein(Local::Service::FileReader& file_reader, int now_timestamp) {
			sprintf(filename_buffer, hourly_filename_template, year(now_timestamp));
			if(!file_reader.open_file_to_read(filename_buffer)) {
				return;
			}

			int i = -1;
			std::uint8_t findings = 0b0000'0001;
			int letzte_gefundene_zeit = 0;
			while(file_reader.read_next_block_to_buffer() && i < stunden_anzahl) {
				if(file_reader.find_in_buffer((char*) "\"EpochDateTime\":([0-9]+)[,}]")) {
					int zeitpunkt = atoi(file_reader.finding_buffer);
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
					&& file_reader.find_in_buffer((char*) "\"SolarIrradiance\":{[^}]*\"Value\":([0-9.]+)[,}]")
				) {
					solarstrahlung_stunden_liste[i] = round(atof(file_reader.finding_buffer));
					findings = 0b0000'0001;
				}
			}
			file_reader.close_file();
		}

		int _timestamp_to_date(int timestamp) {
			return day(timestamp) + (month(timestamp) * 100) + (year(timestamp) * 10000);
		}

		void _lese_tagesdaten_und_setze_ein(Local::Service::FileReader& file_reader, int now_timestamp) {
			sprintf(filename_buffer, dayly_filename_template, year(now_timestamp));
			if(!file_reader.open_file_to_read(filename_buffer)) {
				return;
			}

			int i = -1;
			std::uint8_t findings = 0b0000'0011;
			int letzte_gefundene_zeit = 0;
			int now_date = _timestamp_to_date(now_timestamp);
			zeitpunkt_sonnenuntergang = 0;
			while(file_reader.read_next_block_to_buffer() && i < tage_anzahl) {
				if(
					zeitpunkt_sonnenuntergang == 0
					&& file_reader.find_in_buffer((char*) "\"EpochSet\":([0-9]+)},\"Moon\"")
				) {
					int zeitpunkt = atoi(file_reader.finding_buffer);
					if(zeitpunkt >= now_timestamp || zeitpunkt > now_timestamp - 4 * 3600) {
						zeitpunkt_sonnenuntergang = zeitpunkt;
					}
				}
				if(file_reader.find_in_buffer((char*) "\"EpochDate\":([0-9]+)[,}]")) {
					int zeitpunkt = atoi(file_reader.finding_buffer);
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
					file_reader.find_in_buffer((char*) "\"SolarIrradiance\":{[^}]*\"Value\":([0-9.]+)[,}]")
				) {
					// Tag & Nacht (das sind 2 gleich lautende Keys)
					solarstrahlung_tage_liste[i] += round(atof(file_reader.finding_buffer));
					findings |= 0b0000'0001;
				}
				if(file_reader.find_in_buffer((char*) "\"Night\":")) {
					// Erst wenn der Nacht-Uebergang gefunden wurde, nochmal auslesen erlauben
					findings = 0b0000'0000;
				}
			}
			file_reader.close_file();
		}

		void _lese_stundencache_und_setze_ein(Local::Service::FileReader& file_reader, int now_timestamp) {
			sprintf(filename_buffer, hourly_cache_filename_template, year(now_timestamp));
			if(!file_reader.open_file_to_read(filename_buffer)) {
				return;
			}
			int i = 0;
			while(file_reader.read_next_line_to_buffer() && i < stunden_anzahl) {
				if(file_reader.find_in_buffer((char*) "^([0-9]+),([0-9]+)$")) {
					int cache_zeit = atoi(file_reader.finding_buffer);
					if(now_timestamp - 1800 > cache_zeit) {
						continue;// Zu alt, ueberspringen
					}
					zeitpunkt_stunden_liste[i] = cache_zeit;
					file_reader.fetch_next_finding();
					solarstrahlung_stunden_liste[i] = atoi(file_reader.finding_buffer);
					i++;
				}
			}
			file_reader.close_file();
		}

		void _lese_tagescache_und_setze_ein(Local::Service::FileReader& file_reader, int now_timestamp) {
			sprintf(filename_buffer, dayly_cache_filename_template, year(now_timestamp));
			if(!file_reader.open_file_to_read(filename_buffer)) {
				return;
			}
			int i = 0;
			int now_date = _timestamp_to_date(now_timestamp);
			while(file_reader.read_next_line_to_buffer() && i < tage_anzahl) {
				if(file_reader.find_in_buffer((char*) "^([0-9]+),([0-9]+)$")) {
					int cache_zeit = atoi(file_reader.finding_buffer);
					if(_timestamp_to_date(cache_zeit) < now_date) {
						continue;// Zu alt, ueberspringen
					}
					zeitpunkt_tage_liste[i] = cache_zeit;
					file_reader.fetch_next_finding();
					solarstrahlung_tage_liste[i] = atoi(file_reader.finding_buffer);
					i++;
				}
			}
			file_reader.close_file();
		}

		void _schreibe_stundencache(Local::Service::FileWriter& file_writer, int now_timestamp) {
			sprintf(filename_buffer, hourly_cache_filename_template, year(now_timestamp));
			if(!file_writer.open_file_to_overwrite(filename_buffer)) {
				return;
			}
			for(int i = 0; i < stunden_anzahl; i++) {
				file_writer.write_formated(
					"%d,%d\n",
					zeitpunkt_stunden_liste[i],
					solarstrahlung_stunden_liste[i]
				);
			}
			file_writer.close_file();
		}

		void _schreibe_tagescache(Local::Service::FileWriter& file_writer, int now_timestamp) {
			sprintf(filename_buffer, dayly_cache_filename_template, year(now_timestamp));
			if(!file_writer.open_file_to_overwrite(filename_buffer)) {
				return;
			}
			for(int i = 0; i < tage_anzahl; i++) {
				file_writer.write_formated(
					"%d,%d\n",
					zeitpunkt_tage_liste[i],
					solarstrahlung_tage_liste[i]
				);
			}
			file_writer.close_file();
		}

	public:
		void daten_holen_und_persistieren(
			Local::Service::FileReader& file_reader,
			Local::Service::FileWriter& file_writer,
			int now_timestamp
		) {
			sprintf(filename_buffer, roof1_filename_template, year(now_timestamp));
			_daten_holen_und_persistieren(
				file_reader, file_writer, filename_buffer, now_timestamp,
				cfg->wettervorhersage_dach1_neigung_in_grad,
				cfg->wettervorhersage_dach1_ausrichtung_azimuth
			);

			sprintf(filename_buffer, roof2_filename_template, year(now_timestamp));
			_daten_holen_und_persistieren(
				file_reader, file_writer, filename_buffer, now_timestamp,
				cfg->wettervorhersage_dach1_neigung_in_grad,
				cfg->wettervorhersage_dach1_ausrichtung_azimuth
			);
		}

		void persistierte_daten_einsetzen(
			Local::Service::FileReader& file_reader,
			Local::Service::FileWriter& file_writer,
			Local::Model::Wetter& wetter, int now_timestamp
		) {
			_reset(zeitpunkt_stunden_liste, stunden_anzahl);
			_reset(solarstrahlung_stunden_liste, stunden_anzahl);
			wetter.stundenvorhersage_startzeitpunkt = 0;
			for(int i = 0; i < stunden_anzahl; i++) {
				wetter.setze_stundenvorhersage_solarstrahlung(i, 0);
			}

			_lese_stundencache_und_setze_ein(file_reader, now_timestamp);
			_lese_stundendaten_und_setze_ein(file_reader, now_timestamp);
			wetter.stundenvorhersage_startzeitpunkt = zeitpunkt_stunden_liste[0];
			for(int i = 0; i < stunden_anzahl; i++) {
				wetter.setze_stundenvorhersage_solarstrahlung(i, solarstrahlung_stunden_liste[i]);
			}
			_schreibe_stundencache(file_writer, now_timestamp);

			_reset(zeitpunkt_tage_liste, tage_anzahl);
			_reset(solarstrahlung_tage_liste, tage_anzahl);
			wetter.tagesvorhersage_startzeitpunkt = 0;
			wetter.zeitpunkt_sonnenuntergang = 0;
			for(int i = 0; i < tage_anzahl; i++) {
				wetter.setze_tagesvorhersage_solarstrahlung(i, 0);
			}

			_lese_tagescache_und_setze_ein(file_reader, now_timestamp);
			_lese_tagesdaten_und_setze_ein(file_reader, now_timestamp);
			wetter.tagesvorhersage_startzeitpunkt = zeitpunkt_tage_liste[0];
			wetter.zeitpunkt_sonnenuntergang = zeitpunkt_sonnenuntergang;
			for(int i = 0; i < tage_anzahl; i++) {
				wetter.setze_tagesvorhersage_solarstrahlung(i, solarstrahlung_tage_liste[i]);
			}
			_schreibe_tagescache(file_writer, now_timestamp);
		}
	};
}
