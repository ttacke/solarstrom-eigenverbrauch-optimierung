#pragma once
#include "base_api.h"

namespace Local {
	class VerbraucherAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		int timestamp;
		Local::Persistenz* persistenz;

		const char* heizung_relay_zustand_seit_filename = "heizung_relay.status";

		const char* wasser_relay_zustand_seit_filename = "wasser_relay.status";

		const char* roller_relay_zustand_seit_filename = "roller_relay.zustand_seit";
		const char* roller_relay_status_filename = "roller_relay.status";
		int roller_benoetigte_leistung_in_w;
		const char* roller_leistung_filename = "roller_leistung.status";
		const char* roller_leistung_log_filename = "roller_leistung.log";

		const char* auto_relay_zustand_seit_filename = "auto_relay.zustand_seit";
		const char* auto_relay_status_filename = "auto_relay.status";
		int auto_benoetigte_leistung_in_w;
		const char* auto_leistung_filename = "auto_leistung.status";
		const char* auto_leistung_log_filename = "auto_leistung.log";

		const char* ueberschuss_leistung_log_filename = "ueberschuss_leistung.log";

/*
		// TODO erst mal nur Auto
		// Roller arbeitet einfach parallel
		behandle_logge_in_datei(l3_in_ma);// nur 5 slots (lesen, einfügen, >5 entfernen, schreiben)
		behandle_logge_in_datei(ueberschuss_in_w);// nur 5 slots (lesen, einfügen, >5 entfernen, schreiben)

		auto_relay_in_zustand_seit = lade(datei) // Immer beim schalten zeitpunkt schreiben
		lade_mindestdauer_ist_erreicht = now - auto_relay_in_zustand_seit >= 10 * 60; //Mindestlaufzeit: 10min
		if(
			auto_relay_ist_an
			&& lade_mindestdauer_ist_erreicht
			&& letzte_5_logs_l3_in_ma mindestens 2x < x*0.9
		) {
			setze_auto_ladestatus(Local::Verbraucher::Ladestatus::off);
		}

		lade_mindestdauer_ist_erreicht = now - auto_relay_in_zustand_seit >= 10 * 60;
		if(auto_ladestatus = solar) {
			if(
				auto_relay_ist_an
				&& lade_mindestdauer_ist_erreicht
				&& ueberschuss_in_w_letzte_5logs mindestens 3x < auto_benoetigte_leistung_in_w
				&& akku_ladestand_in_promille < 600
			) {
				_schalte_auto_relay(false);
			} else if(
				!auto_relay_ist_an
				&& (
					ueberschuss_in_w_letzte_5logs mindestens 3x > auto_benoetigte_leistung_in_w
					|| akku_ladestand_in_promille > 700
				)
			) {
				_schalte_auto_relay(true);
				// TODO alle anderen Elemente duerfen erst in 5min geschalten werden!
			}
		}
*/



		void _ermittle_relay_zustaende(Local::Verbraucher& verbraucher) {
			verbraucher.heizung_relay_ist_an = _netz_relay_ist_an(cfg->heizung_relay_host, cfg->heizung_relay_port);
			verbraucher.heizung_relay_zustand_seit = _lese_zustand_seit(heizung_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher.wasser_relay_ist_an = _netz_relay_ist_an(cfg->wasser_relay_host, cfg->wasser_relay_port);
			verbraucher.wasser_relay_zustand_seit = _lese_zustand_seit(wasser_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher.auto_relay_ist_an = _netz_relay_ist_an(cfg->auto_relay_host, cfg->auto_relay_port);
			verbraucher.auto_relay_zustand_seit = _lese_zustand_seit(auto_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher.roller_relay_ist_an = _shellyplug_ist_an(cfg->roller_relay_host, cfg->roller_relay_port);
			verbraucher.roller_relay_zustand_seit = _lese_zustand_seit(roller_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
		}

		bool _netz_relay_ist_an(const char* host, int port) {
			web_client->send_http_get_request(
				host,
				port,
				"/relay"
			);
			while(web_client->read_next_block_to_buffer()) {
				if(web_client->find_in_content((char*) "\"ison\":true")) {
					return true;
				}
			}
			return false;
		}

		void _schalte_netz_relay(bool ein, const char* host, int port) {
			web_client->send_http_get_request(
				host,
				port,
				(ein ? "/relay?set_relay=true" : "/relay?set_relay=false")
			);
		}

		bool _shellyplug_ist_an(const char* host, int port) {
			web_client->send_http_get_request(
				host,
				port,
				"/status"
			);
			while(web_client->read_next_block_to_buffer()) {
				if(web_client->find_in_content((char*) "\"ison\":true")) {
					return true;
				}
			}
			return false;
		}

		void _schalte_shellyplug(bool ein, const char* host, int port) {
			web_client->send_http_get_request(
				host,
				port,
				(ein ? "/relay/0?turn=on" : "/relay/0?turn=off")
			);
		}

		int _gib_aktuelle_shellyplug_leistung(const char* host, int port) {
			web_client->send_http_get_request(
				host,
				port,
				"/status"
			);
			while(web_client->read_next_block_to_buffer()) {
				if(web_client->find_in_content((char*) "\"power\":([0-9]+)")) {
					return atoi(web_client->finding_buffer);
				}
			}
			return 0;
		}

		int _gib_auto_benoetigte_leistung_in_w() {
			int leistung = cfg->auto_benoetigte_leistung_gering_in_w;
			if(persistenz->open_file_to_read(auto_leistung_filename)) {
				while(persistenz->read_next_block_to_buffer()) {
					if(persistenz->find_in_content((char*) "([0-9]+)")) {
						int i = atoi(persistenz->finding_buffer);
						if(i > 0) {
							leistung = i;
						}
					}
				}
				persistenz->close_file();
			}
			return leistung;
		}

		void _schalte_roller_relay(bool ein) {
			_schalte_shellyplug(ein, cfg->roller_relay_host, cfg->roller_relay_port);
			if(persistenz->open_file_to_overwrite(roller_relay_zustand_seit_filename)) {
				sprintf(persistenz->buffer, "%d", timestamp);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
		}

		void _schalte_auto_relay(bool ein) {
			_schalte_netz_relay(ein, cfg->auto_relay_host, cfg->auto_relay_port);
			if(persistenz->open_file_to_overwrite(auto_relay_zustand_seit_filename)) {
				sprintf(persistenz->buffer, "%d", timestamp);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
		}

		int _lese_zustand_seit(const char* filename) {
			int seit = 0;
			if(persistenz->open_file_to_read(filename)) {
				while(persistenz->read_next_block_to_buffer()) {
					if(persistenz->find_in_content((char*) "([0-9]+)")) {
						seit = atoi(persistenz->finding_buffer);
					}
				}
				persistenz->close_file();
			}
			return seit;
		}

		int _gib_roller_benoetigte_leistung_in_w() {
			int leistung = cfg->roller_benoetigte_leistung_hoch_in_w;
			if(persistenz->open_file_to_read(roller_leistung_filename)) {
				while(persistenz->read_next_block_to_buffer()) {
					if(persistenz->find_in_content((char*) "([0-9]+)")) {
						int i = atoi(persistenz->finding_buffer);
						if(i > 0) {
							leistung = i;
						}
					}
				}
				persistenz->close_file();
			}
			return leistung;
		}

	public:
		VerbraucherAPI(
			Local::Config& cfg,
			Local::WebClient& web_client,
			Local::Persistenz& persistenz,
			int timestamp
		): BaseAPI(cfg, web_client), persistenz(&persistenz), timestamp(timestamp),
			roller_benoetigte_leistung_in_w(_gib_roller_benoetigte_leistung_in_w()),
			auto_benoetigte_leistung_in_w(_gib_auto_benoetigte_leistung_in_w())
		{
		}

		void _lies_verbraucher_log(int* liste, const char* log_filename) {
			// TODO Datei lesen und 5 Elemente in liste setzen
			for(int i = 0; i < 5; i++) {
//				liste[i] =
			}
		}

		void daten_holen_und_einsetzen(
			Local::Verbraucher& verbraucher,
			Local::ElektroAnlage& elektroanlage,
			Local::Wetter wetter
		) {
			_ermittle_relay_zustaende(verbraucher);

			verbraucher.aktuelle_auto_ladeleistung_in_w = round(elektroanlage.l3_strom_ma / 1000 * 230);
			_lies_verbraucher_log(verbraucher.auto_leistung_log_in_w, auto_leistung_log_filename);

			verbraucher.aktuelle_roller_ladeleistung_in_w = _gib_aktuelle_shellyplug_leistung(cfg->roller_relay_host, cfg->roller_relay_port);
			_lies_verbraucher_log(verbraucher.roller_leistung_log_in_w, roller_leistung_log_filename);

			verbraucher.aktueller_ueberschuss_in_w = elektroanlage.gib_ueberschuss_in_w();
			_lies_verbraucher_log(verbraucher.ueberschuss_log_in_w, ueberschuss_leistung_log_filename);

			verbraucher.aktueller_akku_ladenstand_in_promille = elektroanlage.solarakku_ladestand_in_promille;

			// TODO status==force && relay aus -> status aus!
			// TODO status aus File laden und setzen
			verbraucher.auto_ladestatus = Local::Verbraucher::Ladestatus::off;
			if(verbraucher.auto_relay_ist_an) {
				verbraucher.auto_ladestatus = Local::Verbraucher::Ladestatus::force;
			}
			verbraucher.roller_ladestatus = Local::Verbraucher::Ladestatus::off;
			if(verbraucher.roller_relay_ist_an) {
				verbraucher.roller_ladestatus = Local::Verbraucher::Ladestatus::force;
			}
			verbraucher.auto_benoetigte_leistung_in_w = auto_benoetigte_leistung_in_w;
			verbraucher.roller_ladeleistung_in_w = roller_benoetigte_leistung_in_w;
		}

		void setze_roller_ladestatus(Local::Verbraucher::Ladestatus status) {
			char stat[6];
			if(status == Local::Verbraucher::Ladestatus::force) {
				_schalte_roller_relay(true);
				strcpy(stat, "force");
			} else if(status == Local::Verbraucher::Ladestatus::solar) {
				_schalte_roller_relay(false);
				strcpy(stat, "solar");
			} else {
				_schalte_roller_relay(false);
				strcpy(stat, "off");
			}
			if(persistenz->open_file_to_overwrite(roller_relay_status_filename)) {
				sprintf(persistenz->buffer, "%s,%d", stat, timestamp);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
			if(persistenz->open_file_to_overwrite(roller_leistung_log_filename)) {
				persistenz->close_file();
			}
		}

		void setze_auto_ladestatus(Local::Verbraucher::Ladestatus status) {
			char stat[6];
			if(status == Local::Verbraucher::Ladestatus::force) {
				_schalte_auto_relay(true);
				strcpy(stat, "force");
			} else if(status == Local::Verbraucher::Ladestatus::solar) {
				_schalte_auto_relay(false);
				strcpy(stat, "solar");
			} else {
				_schalte_auto_relay(false);
				strcpy(stat, "off");
			}
			if(persistenz->open_file_to_overwrite(auto_relay_status_filename)) {
				sprintf(persistenz->buffer, "%s,%d", stat, timestamp);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
			if(persistenz->open_file_to_overwrite(auto_leistung_log_filename)) {
				persistenz->close_file();
			}
		}

		void wechsle_auto_ladeleistung() {
			auto_benoetigte_leistung_in_w = _gib_auto_benoetigte_leistung_in_w();
			if(auto_benoetigte_leistung_in_w == cfg->auto_benoetigte_leistung_hoch_in_w) {
				auto_benoetigte_leistung_in_w = cfg->auto_benoetigte_leistung_gering_in_w;
			} else {
				auto_benoetigte_leistung_in_w = cfg->auto_benoetigte_leistung_hoch_in_w;
			}
			if(persistenz->open_file_to_overwrite(auto_leistung_filename)) {
				sprintf(persistenz->buffer, "%d", auto_benoetigte_leistung_in_w);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
		}

		void wechsle_roller_ladeleistung() {
			roller_benoetigte_leistung_in_w = _gib_roller_benoetigte_leistung_in_w();
			if(roller_benoetigte_leistung_in_w == cfg->roller_benoetigte_leistung_hoch_in_w) {
				roller_benoetigte_leistung_in_w = cfg->roller_benoetigte_leistung_gering_in_w;
			} else {
				roller_benoetigte_leistung_in_w = cfg->roller_benoetigte_leistung_hoch_in_w;
			}
			if(persistenz->open_file_to_overwrite(roller_leistung_filename)) {
				sprintf(persistenz->buffer, "%d", roller_benoetigte_leistung_in_w);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
		}
	};
}
