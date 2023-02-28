#pragma once
#include "base_api.h"

namespace Local {
	class VerbraucherAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		int timestamp;
		Local::Persistenz* persistenz;

		bool heizung_relay_ist_an = false;
		int heizung_relay_zustand_seit = 0;
		const char* heizung_relay_zustand_seit_filename = "heizung_relay.status";

		bool wasser_relay_ist_an = false;
		int wasser_relay_zustand_seit = 0;
		const char* wasser_relay_zustand_seit_filename = "wasser_relay.status";

		bool roller_relay_ist_an = false;
		int roller_relay_zustand_seit = 0;
		const char* roller_relay_zustand_seit_filename = "roller_relay.zustand_seit";
		const char* roller_relay_status_filename = "roller_relay.status";
		int roller_benoetigte_leistung_in_w;
		const char* roller_leistung_filename = "roller_leistung.status";
		int roller_leistung_log_in_w[5];
		const char* roller_leistung_log_filename = "roller_leistung.log";

		bool auto_relay_ist_an = false;
		int auto_relay_zustand_seit = 0;
		const char* auto_relay_zustand_seit_filename = "auto_relay.zustand_seit";
		const char* auto_relay_status_filename = "auto_relay.status";
		int auto_benoetigte_leistung_in_w;
		const char* auto_leistung_filename = "auto_leistung.status";
		int auto_leistung_log_in_w[5];
		const char* auto_leistung_log_filename = "auto_leistung.log";

		int ueberschuss_log_in_w[5];


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



		void _ermittle_alle_zustaende() {
			heizung_relay_ist_an = _netz_relay_ist_an(cfg->heizung_relay_host, cfg->heizung_relay_port);
			heizung_relay_zustand_seit = _lese_zustand_seit(heizung_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			wasser_relay_ist_an = _netz_relay_ist_an(cfg->wasser_relay_host, cfg->wasser_relay_port);
			wasser_relay_zustand_seit = _lese_zustand_seit(wasser_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			auto_relay_ist_an = _netz_relay_ist_an(cfg->auto_relay_host, cfg->auto_relay_port);
			auto_relay_zustand_seit = _lese_zustand_seit(auto_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			roller_relay_ist_an = _shellyplug_ist_an(cfg->roller_relay_host, cfg->roller_relay_port);
			roller_relay_zustand_seit = _lese_zustand_seit(roller_relay_zustand_seit_filename);
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
			roller_relay_ist_an = ein;
			roller_relay_zustand_seit = timestamp;
			if(persistenz->open_file_to_overwrite(roller_relay_zustand_seit_filename)) {
				sprintf(persistenz->buffer, "%d", roller_relay_zustand_seit);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
		}

		void _schalte_auto_relay(bool ein) {
			_schalte_netz_relay(ein, cfg->auto_relay_host, cfg->auto_relay_port);
			auto_relay_ist_an = ein;
			auto_relay_zustand_seit = timestamp;
			if(persistenz->open_file_to_overwrite(auto_relay_zustand_seit_filename)) {
				sprintf(persistenz->buffer, "%d", auto_relay_zustand_seit);
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

		void daten_holen_und_einsetzen(
			Local::Verbraucher& verbraucher,
			Local::ElektroAnlage& elektroanlage,
			Local::Wetter wetter
		) {
			_ermittle_alle_zustaende();
			//elektroanlage.l3_strom_ma

//			int ueberschuss_in_w = elektroanlage.gib_ueberschuss_in_w();
//			elektroanlage.solarakku_ladestand_in_promille
//			elektroanlage.l3_strom_ma (= auto hängt dran)
//			elektroanlage.gib_ueberschuss_in_w()

			verbraucher.heizung_ueberladen_ist_an = heizung_relay_ist_an;
			verbraucher.wasser_ueberladen_ist_an = wasser_relay_ist_an;
			verbraucher.auto_ladestatus = Local::Verbraucher::Ladestatus::off;
			if(auto_relay_ist_an) {
				verbraucher.auto_ladestatus = Local::Verbraucher::Ladestatus::force;
			}
			verbraucher.roller_ladestatus = Local::Verbraucher::Ladestatus::off;
			if(roller_relay_ist_an) {
				verbraucher.roller_ladestatus = Local::Verbraucher::Ladestatus::force;
			}
			verbraucher.auto_ladeleistung_in_w = auto_benoetigte_leistung_in_w;
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
