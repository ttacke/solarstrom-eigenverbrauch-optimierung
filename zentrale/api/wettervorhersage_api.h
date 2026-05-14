#pragma once
#include "base_api.h"
#include "../model/wetter.h"
#include <TimeLib.h>

namespace Local::Api {
	class WettervorhersageAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		const char* request_uri_template = "/v1/forecast?latitude=%0.2f&longitude=%0.2f&daily=sunrise,sunset,shortwave_radiation_sum&hourly=global_tilted_irradiance_instant&timezone=Europe/Berlin&tilt=%d&azimuth=%d&timeformat=unixtime&forecast_hours=12";
		char request_uri_buffer[256];

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

		bool _daten_holen_und_parsen(int neigung_in_grad, int azimuth, int now_timestamp) {
			sprintf(
				request_uri_buffer,
				request_uri_template,
				cfg->wettervorhersage_lat,
				cfg->wettervorhersage_lon,
				neigung_in_grad,
				azimuth
			);
			if(!web_reader->send_http_get_request(
				"api.open-meteo.com",
				80,
				request_uri_buffer
			)) {
				Serial.println("FEHLER Open-Meteo-Request");
				return false;
			}

			int veraltete_stunden_datensaetze = 0;
			int veraltete_tages_datensaetze = 0;
			bool searching_dayly_radiation = false;
			bool searching_dayly_time = false;
			bool searching_hourly_radiation = false;
			bool searching_hourly_time = false;
			while(web_reader->read_next_block_to_buffer()) {
				if(
					zeitpunkt_sonnenuntergang == 0
					&& web_reader->find_in_buffer((char*) "\"sunset\":[^0-9]([0-9]+)[^0-9]")
				) {
					int zeitpunkt = atoi(web_reader->finding_buffer);
					if(zeitpunkt >= now_timestamp || zeitpunkt > now_timestamp - 4 * 3600) {
						zeitpunkt_sonnenuntergang = zeitpunkt;
					}
				}

				if(web_reader->find_in_buffer((char*) "(\"daily\":{)")) {
					searching_dayly_radiation = true;
					searching_dayly_time = true;
				}
				if(searching_dayly_time) {
					if(web_reader->find_in_buffer((char*) "\"time\":[^0-9]([0-9]+)[^0-9]")) {
						searching_dayly_time = false;
						veraltete_tages_datensaetze = 0;
						int zeitpunkt = atoi(web_reader->finding_buffer);
						for(int i = 0; i < tage_anzahl; i++) {
							zeitpunkt_tage_liste[i] = zeitpunkt + (i * 86400);
							if(now_timestamp > zeitpunkt_tage_liste[i] + 86400) {
								veraltete_tages_datensaetze++;
							}
						}
					}
				}
				if(searching_dayly_radiation) {
					if(
						web_reader->find_in_buffer(
							(char*) "\"shortwave_radiation_sum\":[^0-9]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]"
						)
					) {
						searching_dayly_radiation = false;
						int i = 0;
						solarstrahlung_tage_liste[i] = round(
							atof(web_reader->finding_buffer)
							* cfg->tageswert_anpassung
						);
						while(web_reader->fetch_next_finding() && i < tage_anzahl - 1) {
							i++;
							solarstrahlung_tage_liste[i] = round(
								atoi(web_reader->finding_buffer)
								* cfg->tageswert_anpassung
							);
						}
					}
				}

				if(web_reader->find_in_buffer((char*) "(\"hourly\":{)")) {
					searching_hourly_radiation = true;
					searching_hourly_time = true;
				}
				if(searching_hourly_time) {
					if(web_reader->find_in_buffer((char*) "\"time\":[^0-9]([0-9]+)[^0-9]")) {
						searching_hourly_time = false;
						veraltete_stunden_datensaetze = 0;
						int zeitpunkt = atoi(web_reader->finding_buffer);
						for(int i = 0; i < stunden_anzahl; i++) {
							zeitpunkt_stunden_liste[i] = zeitpunkt + (i * 3600);
							if(now_timestamp > zeitpunkt_stunden_liste[i] + 1800) {
								veraltete_stunden_datensaetze++;
							}
						}
					}
				}
				if(searching_hourly_radiation) {
					if(
						web_reader->find_in_buffer(
							(char*) "instant\":[^0-9]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]([0-9.]+)[^0-9.]"
						)
					) {
						searching_hourly_radiation = false;
						int i = 0;
						solarstrahlung_stunden_liste[i] = round(
							atof(web_reader->finding_buffer)
							* cfg->stundenwert_anpassung
						);
						while(web_reader->fetch_next_finding() && i < stunden_anzahl - 1) {
							i++;
							solarstrahlung_stunden_liste[i] = round(
								atoi(web_reader->finding_buffer)
								* cfg->stundenwert_anpassung
							);
						}
					}
				}
			}
			while(veraltete_tages_datensaetze > 0) {
				veraltete_tages_datensaetze--;
				zeitpunkt_tage_liste[tage_anzahl - 1] = 0;
				solarstrahlung_tage_liste[tage_anzahl - 1] = 0;
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
			return true;
		}

		int _timestamp_to_date(int timestamp) {
			return day(timestamp) + (month(timestamp) * 100) + (year(timestamp) * 10000);
		}

		void _lese_stundencache_und_setze_ein(int now_timestamp) {
			int i = 0;
			for(int j = 0; j < stunden_anzahl && i < stunden_anzahl; j++) {
				int cache_zeit = Local::SemipersistentData::wetter_stundencache_zeitpunkt[j];
				if(cache_zeit == 0 || now_timestamp - 1800 > cache_zeit) {
					continue;// Zu alt oder leer, ueberspringen
				}
				zeitpunkt_stunden_liste[i] = cache_zeit;
				solarstrahlung_stunden_liste[i] = Local::SemipersistentData::wetter_stundencache_solarstrahlung[j];
				i++;
			}
		}

		void _lese_tagescache_und_setze_ein(int now_timestamp) {
			int i = 0;
			int now_date = _timestamp_to_date(now_timestamp);
			for(int j = 0; j < tage_anzahl && i < tage_anzahl; j++) {
				int cache_zeit = Local::SemipersistentData::wetter_tagescache_zeitpunkt[j];
				if(cache_zeit == 0 || _timestamp_to_date(cache_zeit) < now_date) {
					continue;// Zu alt oder leer, ueberspringen
				}
				zeitpunkt_tage_liste[i] = cache_zeit;
				solarstrahlung_tage_liste[i] = Local::SemipersistentData::wetter_tagescache_solarstrahlung[j];
				i++;
			}
		}

		void _schreibe_stundencache() {
			for(int i = 0; i < stunden_anzahl; i++) {
				Local::SemipersistentData::wetter_stundencache_zeitpunkt[i] = zeitpunkt_stunden_liste[i];
				Local::SemipersistentData::wetter_stundencache_solarstrahlung[i] = solarstrahlung_stunden_liste[i];
			}
		}

		void _schreibe_tagescache() {
			for(int i = 0; i < tage_anzahl; i++) {
				Local::SemipersistentData::wetter_tagescache_zeitpunkt[i] = zeitpunkt_tage_liste[i];
				Local::SemipersistentData::wetter_tagescache_solarstrahlung[i] = solarstrahlung_tage_liste[i];
			}
		}

	public:
		void daten_holen_und_persistieren(int now_timestamp) {
			_reset(zeitpunkt_stunden_liste, stunden_anzahl);
			_reset(solarstrahlung_stunden_liste, stunden_anzahl);
			_reset(zeitpunkt_tage_liste, tage_anzahl);
			_reset(solarstrahlung_tage_liste, tage_anzahl);
			zeitpunkt_sonnenuntergang = 0;

			if(!_daten_holen_und_parsen(
				cfg->wettervorhersage_dach1_neigung_in_grad,
				cfg->wettervorhersage_dach1_ausrichtung_azimuth,
				now_timestamp
			)) {
				return;
			}

			int temp[12];
			for(int i = 0; i < stunden_anzahl; i++) {
				temp[i] = solarstrahlung_stunden_liste[i];
			}

			if(!_daten_holen_und_parsen(
				cfg->wettervorhersage_dach2_neigung_in_grad,
				cfg->wettervorhersage_dach2_ausrichtung_azimuth,
				now_timestamp
			)) {
				return;
			}

			for(int i = 0; i < stunden_anzahl; i++) {
				solarstrahlung_stunden_liste[i] = round((temp[i] + solarstrahlung_stunden_liste[i]) / 2);
			}

			_schreibe_stundencache();
			_schreibe_tagescache();
			Local::SemipersistentData::wetter_zeitpunkt_sonnenuntergang = zeitpunkt_sonnenuntergang;
		}

		void persistierte_daten_einsetzen(Local::Model::Wetter& wetter, int now_timestamp) {
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

			_lese_stundencache_und_setze_ein(now_timestamp);
			_lese_tagescache_und_setze_ein(now_timestamp);

			wetter.stundenvorhersage_startzeitpunkt = zeitpunkt_stunden_liste[0];
			for(int i = 0; i < stunden_anzahl; i++) {
				wetter.setze_stundenvorhersage_solarstrahlung(i, solarstrahlung_stunden_liste[i]);
			}

			wetter.tagesvorhersage_startzeitpunkt = zeitpunkt_tage_liste[0];
			wetter.zeitpunkt_sonnenuntergang = Local::SemipersistentData::wetter_zeitpunkt_sonnenuntergang;
			for(int i = 0; i < tage_anzahl; i++) {
				wetter.setze_tagesvorhersage_solarstrahlung(i, solarstrahlung_tage_liste[i]);
			}
		}
	};
}
