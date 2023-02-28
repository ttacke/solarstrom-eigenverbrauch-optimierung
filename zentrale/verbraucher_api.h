#pragma once
#include "base_api.h"

namespace Local {
	class VerbraucherAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		int timestamp;
		Local::Persistenz* persistenz;

		bool heizung_relay_ist_an = false;
		const char* heizung_relay_status_filename = "heizung_relay.status";

		bool wasser_relay_ist_an = false;
		const char* wasser_relay_status_filename = "wasser_relay.status";

		bool roller_relay_ist_an = false;
		int roller_relay_ist_an_seit = 0;
		const char* roller_relay_status_filename = "roller_relay.status";
		int roller_benoetigte_leistung_in_w;

		bool auto_relay_ist_an = false;
		int auto_relay_ist_an_seit = 0;
		const char* auto_relay_status_filename = "auto_relay.status";
		int auto_benoetigte_leistung_in_w;


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
			schalte(aus)// Auto Aus wenn Laden unterbrochen/fertig (force & solar)
			schreibe wunsch in datei -> off
			// --> die 2 sachen direkt beim Schalter machen + Zeit in Datei schreiben
			auto_relay_ist_an = false;
			auto_relay_in_zustand_seit = now;
		}

		lade_mindestdauer_ist_erreicht = now - auto_relay_in_zustand_seit >= 10 * 60;
		if(auto_ladestatus = solar) {
			if(
				auto_relay_ist_an
				&& lade_mindestdauer_ist_erreicht
				&& ueberschuss_in_w_letzte_5logs mindestens 3x < auto_benoetigte_leistung_in_w
				&& akku_ladestand_in_promille < 600
			) {
				schalte(aus);
				auto_relay_ist_an = false;
				auto_relay_in_zustand_seit = now;
			} else if(
				!auto_relay_ist_an
				&& (
					ueberschuss_in_w_letzte_5logs mindestens 3x > auto_benoetigte_leistung_in_w
					|| akku_ladestand_in_promille > 700
				)
			) {
				schalte(ein);
				auto_relay_ist_an = false;
				auto_relay_in_zustand_seit = now;
				// WICHTIG: damit Nachfolgende nicht von falschen Werten ausgehen
				ueberschuss_in_w -= auto_benoetigte_leistung_in_w;
			}
		}
*/









		void _lies_status(int strom_auf_auto_leitung_in_ma) {
			heizung_relay_ist_an = _netz_relay_ist_an(cfg->heizung_relay_host, cfg->heizung_relay_port);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
			wasser_relay_ist_an = _netz_relay_ist_an(cfg->wasser_relay_host, cfg->wasser_relay_port);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			auto_relay_ist_an = _netz_relay_ist_an(cfg->auto_relay_host, cfg->auto_relay_port);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
//			if(auto_relay_ist_an) {
//				if(strom_auf_auto_leitung_in_ma > auto_benoetigte_leistung_in_w * 220 * 0.9) {
					// TODO KEINE Stichprobe! Das muss aus der Ladelog kommen -> weil unscharf
					// d.h. bei relay-an immer Loggen
//					aktuelle_auto_ladeleistung_in_w = auto_benoetigte_leistung_in_w * 0.9;
//				} else {
//					aktuelle_auto_ladeleistung_in_w = 0;
//				}
//			}

			roller_relay_ist_an = _shellyplug_ist_an(cfg->roller_relay_host, cfg->roller_relay_port);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
//			if(roller_relay_ist_an) {
//				aktuelle_roller_ladeleisung_in_w = _gib_aktuelle_shellyplug_leistung(cfg->roller_relay_host, cfg->roller_relay_port);
				// TODO KEINE Stichprobe! Das muss aus der Ladelog kommen -> weil Akku einige Sekunden zum start braucht
				// d.h. bei relay-an immer Loggen
//			} else {
//				aktuelle_roller_ladeleisung_in_w = 0;
//			}
//			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
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

	public:
		VerbraucherAPI(
			Local::Config& cfg,
			Local::WebClient& web_client,
			Local::Persistenz& persistenz,
			int timestamp
		): BaseAPI(cfg, web_client), persistenz(&persistenz), timestamp(timestamp),
			auto_benoetigte_leistung_in_w(cfg.auto_benoetigte_leistung_gering_in_w),
			roller_benoetigte_leistung_in_w(cfg.roller_benoetigte_leistung_hoch_in_w)
		{
		}

		void daten_holen_und_einsetzen(
			Local::Verbraucher& verbraucher,
			Local::ElektroAnlage& elektroanlage,
			Local::Wetter wetter
		) {
			_lies_status(elektroanlage.l3_strom_ma);

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
			// TODO wunsch-status in file speichern mit Zeitstempel
			// TODO ladelog leeren
			// TODO schalten passiert im "heartbeat", nicht hier
			if(status == Local::Verbraucher::Ladestatus::force) {
				_schalte_roller_relay(true);
			} else {
				_schalte_roller_relay(false);
			}
		}

		void _schalte_roller_relay(bool ein) {
			_schalte_shellyplug(ein, cfg->roller_relay_host, cfg->roller_relay_port);
			roller_relay_ist_an = ein;
			if(ein) {
				roller_relay_ist_an_seit = timestamp;
				if(persistenz->open_file_to_overwrite(roller_relay_status_filename)) {
					sprintf(persistenz->buffer, "%d", roller_relay_ist_an_seit);
					persistenz->print_buffer_to_file();
					persistenz->close_file();
				}
			}
		}

		void setze_auto_ladestatus(Local::Verbraucher::Ladestatus status) {
			// TODO wunsch-status in file speichern mit Zeitstempel
			// TODO ladelog leeren
			// TODO schalten passiert im "heartbeat", nicht hier
			if(status == Local::Verbraucher::Ladestatus::force) {
				_schalte_netz_relay(true, cfg->auto_relay_host, cfg->auto_relay_port);
			} else {
				_schalte_netz_relay(false, cfg->auto_relay_host, cfg->auto_relay_port);
			}
		}

		void wechsle_auto_ladeleistung() {
			// TODO in file schreiben
		}

		void wechsle_roller_ladeleistung() {
			// TODO in file schreiben
		}
	};
}
