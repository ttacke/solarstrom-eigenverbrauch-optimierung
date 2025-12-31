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
		const char* roof1_filename_template = "dach1_wettervorhersage_%04d-%02d.json";
		const char* roof2_filename_template = "dach2_wettervorhersage_%04d-%02d.json";
		const char* hourly_cache_filename_template = "wetter_stundenvorhersage_%04d-%02d.csv";
		const char* dayly_cache_filename_template = "wetter_tagesvorhersage_%04d-%02d.csv";
		const char* request_uri_template = "/v1/forecast?latitude=%0.2f&longitude=%0.2f&daily=sunrise,sunset,shortwave_radiation_sum&hourly=global_tilted_irradiance_instant&timezone=Europe/Berlin&tilt=%d&azimuth=%d&timeformat=unixtime&forecast_hours=12";
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
			const int neigung_in_grad, const int azimuth
		) {
			// geht der Abruf schief, wird die vorherige Datei zerstoehrt.
			// Der entstehende Schaden ist nicht relevant genug, um sich darum zu kuemmern
			if(!file_writer.open_file_to_overwrite(filename)) {
				Serial.println("Schreibfehler!");
				return;
			}
			sprintf(
				request_uri_buffer,
				request_uri_template,
				cfg->wettervorhersage_lat,
				cfg->wettervorhersage_lon,
				neigung_in_grad,
				azimuth
			);
			web_reader->send_http_get_request(
				"api.open-meteo.com",
				80,
				request_uri_buffer
			);
			while(web_reader->read_next_block_to_buffer()) {
				file_writer.write(web_reader->buffer, strlen(web_reader->buffer));
			}
			file_writer.close_file();
		}

		void _lese_daten_und_setze_ein(Local::Service::FileReader& file_reader, const char* filename, int now_timestamp) {
			sprintf(filename_buffer, filename, year(now_timestamp), month(now_timestamp));
			if(!file_reader.open_file_to_read(filename_buffer)) {
				Serial.println("FEHLER Beim Lesen");
				Serial.println(filename_buffer);
				return;
			}

			int veraltete_stunden_datensaetze = 0;
			int veraltete_tages_datensaetze = 0;
			bool searching_dayly_radiation = false;
			bool searching_dayly_time = false;
			bool searching_hourly_radiation = false;
			bool searching_hourly_time = false;
			zeitpunkt_sonnenuntergang = 0;
			while(file_reader.read_next_block_to_buffer()) {
				if(
					zeitpunkt_sonnenuntergang == 0
					&& file_reader.find_in_buffer((char*) "\"sunset\":[^0-9]([0-9]+)[^0-9]")
				) {
					int zeitpunkt = atoi(file_reader.finding_buffer);
					if(zeitpunkt >= now_timestamp || zeitpunkt > now_timestamp - 4 * 3600) {
						zeitpunkt_sonnenuntergang = zeitpunkt;
					}
				}

				if(file_reader.find_in_buffer((char*) "(\"daily\":{)")) { // Ohne Klammern gehts nicht
					searching_dayly_radiation = true;
					searching_dayly_time = true;
				}
				if(searching_dayly_time) {
					if(file_reader.find_in_buffer((char*) "\"time\":[^0-9]([0-9]+)[^0-9]")) {
						searching_dayly_time = false;
						veraltete_tages_datensaetze = 0;// Hier kommt man ggf 2x vorbei, weil der SearchBuffer "zu gross" ist
						int zeitpunkt = atoi(file_reader.finding_buffer);
						for(int i = 0; i < tage_anzahl; i++) {// Nur die erste Zeit finden, die anderen sind immer 1h weiter. Grund: das Finden ist unnoetig aufwendig
							zeitpunkt_tage_liste[i] = zeitpunkt + (i * 86400);
							if(
								now_timestamp > zeitpunkt_stunden_liste[i] + 86400 // Zu altes ueberspringen
							) {
								veraltete_tages_datensaetze++;
							}
						}
					}
				}
				if(searching_dayly_radiation) {
					if(
						file_reader.find_in_buffer(// shortwave_radiation_sum -> verkuerzt, um den Buffer nicht zu ueberforrdern
							(char*) "\"shortwave_radiation_sum\":[^0-9]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]"
						)
					) {
						searching_dayly_radiation = false;
						// Nur den Ganzzahlwert, Nachkommastellen sind irrelevant
						int i = 0;
						solarstrahlung_tage_liste[i] = round(
							atof(file_reader.finding_buffer)
							* cfg->tageswert_anpassung
						);
						while(file_reader.fetch_next_finding() && i < tage_anzahl - 1) {
							i++;
							solarstrahlung_tage_liste[i] = round(
								atoi(file_reader.finding_buffer)
								* cfg->tageswert_anpassung
							);
						}
					}
				}

				if(file_reader.find_in_buffer((char*) "(\"hourly\":{)")) { // Ohne Klammern gehts nicht
					searching_hourly_radiation = true;
					searching_hourly_time = true;
				}
				if(searching_hourly_time) {
					if(file_reader.find_in_buffer((char*) "\"time\":[^0-9]([0-9]+)[^0-9]")) {
						searching_hourly_time = false;
						veraltete_stunden_datensaetze = 0;// Hier kommt man ggf 2x vorbei, weil der SearchBuffer "zu gross" ist
						int zeitpunkt = atoi(file_reader.finding_buffer);
						for(int i = 0; i < stunden_anzahl; i++) {// Nur die erste Zeit finden, die anderen sind immer 1h weiter. Grund: das Finden ist unnoetig aufwendig
							zeitpunkt_stunden_liste[i] = zeitpunkt + (i * 3600);
							if(
								now_timestamp > zeitpunkt_stunden_liste[i] + 1800 // Zu altes ueberspringen
							) {
								veraltete_stunden_datensaetze++;
							}
						}
					}
				}
				if(searching_hourly_radiation) {
					if(
						file_reader.find_in_buffer(// global_tilted_irradiance_instant -> verkuerzt, um den Buffer nicht zu ueberforrdern
							(char*) "instant\":[^0-9]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]"
						)
					) {
						searching_hourly_radiation = false;
						// Nur den Ganzzahlwert, Nachkommastellen sind irrelevant
						int i = 0;
						solarstrahlung_stunden_liste[i] = round(
							atof(file_reader.finding_buffer)
							* cfg->stundenwert_anpassung
						);
						while(file_reader.fetch_next_finding() && i < stunden_anzahl - 1) {
							i++;
							solarstrahlung_stunden_liste[i] = round(
								atoi(file_reader.finding_buffer)
								* cfg->stundenwert_anpassung
							);
						}
					}
				}
			}
			while(veraltete_tages_datensaetze > 0) {
				veraltete_tages_datensaetze--;
				zeitpunkt_tage_liste[stunden_anzahl - 1] = 0;
				solarstrahlung_tage_liste[stunden_anzahl - 1] = 0;
				for(int i = 1; i < tage_anzahl; i++) {
					solarstrahlung_tage_liste[i - 1] = solarstrahlung_tage_liste[i];
					zeitpunkt_tage_liste[i - 1] = zeitpunkt_tage_liste[i];
				}
			}
			while(veraltete_stunden_datensaetze > 0) {
				veraltete_stunden_datensaetze--;
				zeitpunkt_stunden_liste[stunden_anzahl - 1] = 0;
				solarstrahlung_stunden_liste[stunden_anzahl - 1] = 0;
				for(int i = 1; i < stunden_anzahl; i++) {
					solarstrahlung_stunden_liste[i - 1] = solarstrahlung_stunden_liste[i];
					zeitpunkt_stunden_liste[i - 1] = zeitpunkt_stunden_liste[i];
				}
			}
			file_reader.close_file();
		}

		int _timestamp_to_date(int timestamp) {
			return day(timestamp) + (month(timestamp) * 100) + (year(timestamp) * 10000);
		}

		void _lese_stundencache_und_setze_ein(Local::Service::FileReader& file_reader, int now_timestamp) {
			sprintf(filename_buffer, hourly_cache_filename_template, year(now_timestamp), month(now_timestamp));
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
			sprintf(filename_buffer, dayly_cache_filename_template, year(now_timestamp), month(now_timestamp));
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
			sprintf(filename_buffer, hourly_cache_filename_template, year(now_timestamp), month(now_timestamp));
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
			sprintf(filename_buffer, dayly_cache_filename_template, year(now_timestamp), month(now_timestamp));
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
			sprintf(filename_buffer, roof1_filename_template, year(now_timestamp), month(now_timestamp));
			_daten_holen_und_persistieren(
				file_reader, file_writer, filename_buffer, now_timestamp,
				cfg->wettervorhersage_dach1_neigung_in_grad,
				cfg->wettervorhersage_dach1_ausrichtung_azimuth
			);

			sprintf(filename_buffer, roof2_filename_template, year(now_timestamp), month(now_timestamp));
			_daten_holen_und_persistieren(
				file_reader, file_writer, filename_buffer, now_timestamp,
				cfg->wettervorhersage_dach2_neigung_in_grad,
				cfg->wettervorhersage_dach2_ausrichtung_azimuth
			);
		}

		void persistierte_daten_einsetzen(
			Local::Service::FileReader& file_reader,
			Local::Service::FileWriter& file_writer,
			Local::Model::Wetter& wetter, int now_timestamp
		) {
			_reset(zeitpunkt_stunden_liste, stunden_anzahl);
			_reset(solarstrahlung_stunden_liste, stunden_anzahl);
			_reset(zeitpunkt_tage_liste, tage_anzahl);
			_reset(solarstrahlung_tage_liste, tage_anzahl);
			wetter.stundenvorhersage_startzeitpunkt = 0;
			for(int i = 0; i < stunden_anzahl; i++) {
				wetter.setze_stundenvorhersage_solarstrahlung(i, 0);
			}
			wetter.tagesvorhersage_startzeitpunkt = 0;
			wetter.zeitpunkt_sonnenuntergang = 0;
			for(int i = 0; i < tage_anzahl; i++) {
				wetter.setze_tagesvorhersage_solarstrahlung(i, 0);
			}

			_lese_stundencache_und_setze_ein(file_reader, now_timestamp);
			_lese_tagescache_und_setze_ein(file_reader, now_timestamp);

			_lese_daten_und_setze_ein(file_reader, roof1_filename_template, now_timestamp);

			int temp[12];
			for(int i = 0; i < stunden_anzahl; i++) {
				temp[i] = solarstrahlung_stunden_liste[i];
			}
			_lese_daten_und_setze_ein(file_reader, roof2_filename_template, now_timestamp);
			for(int i = 0; i < stunden_anzahl; i++) {
				solarstrahlung_stunden_liste[i] = round((temp[i] + solarstrahlung_stunden_liste[i]) / 2);
			}

			wetter.stundenvorhersage_startzeitpunkt = zeitpunkt_stunden_liste[0];
			for(int i = 0; i < stunden_anzahl; i++) {
				wetter.setze_stundenvorhersage_solarstrahlung(i, solarstrahlung_stunden_liste[i]);
			}
			_schreibe_stundencache(file_writer, now_timestamp);

			wetter.tagesvorhersage_startzeitpunkt = zeitpunkt_tage_liste[0];
			wetter.zeitpunkt_sonnenuntergang = zeitpunkt_sonnenuntergang;
			for(int i = 0; i < tage_anzahl; i++) {
				wetter.setze_tagesvorhersage_solarstrahlung(i, solarstrahlung_tage_liste[i]);
			}
			_schreibe_tagescache(file_writer, now_timestamp);
		}
	};
}
