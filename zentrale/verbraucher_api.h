#pragma once
#include "base_api.h"

namespace Local {
	class VerbraucherAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
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
		void status_anpassen_und_daten_holen_und_einsetzen(Local::Verbraucher& verbraucher, int timestamp) {
			verbraucher.heizung_ueberladen_ist_an = _netz_relay_ist_an(cfg->heizung_ueberladen_host, cfg->heizung_ueberladen_port);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher.wasser_ueberladen_ist_an = _netz_relay_ist_an(cfg->wasser_ueberladen_host, cfg->wasser_ueberladen_port);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			// TODO
			verbraucher.auto_ladestatus = Local::Verbraucher::Ladestatus::off;
			if(_netz_relay_ist_an(cfg->auto_laden_host, cfg->auto_laden_port)) {
				verbraucher.auto_ladestatus = Local::Verbraucher::Ladestatus::force;
			}
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			// TODO
			verbraucher.roller_ladestatus = Local::Verbraucher::Ladestatus::off;
			if(_shellyplug_ist_an(cfg->roller_laden_host, cfg->roller_laden_port)) {
				verbraucher.roller_ladestatus = Local::Verbraucher::Ladestatus::force;
			}
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
		}

		void setze_roller_ladestatus(Local::Verbraucher::Ladestatus status) {
			if(status == Local::Verbraucher::Ladestatus::force) {
				_schalte_shellyplug(true, cfg->roller_laden_host, cfg->roller_laden_port);
			} else {
				_schalte_shellyplug(false, cfg->roller_laden_host, cfg->roller_laden_port);
			}
		}

		void setze_auto_ladestatus(Local::Verbraucher::Ladestatus status) {
			if(status == Local::Verbraucher::Ladestatus::force) {
				_schalte_netz_relay(true, cfg->auto_laden_host, cfg->auto_laden_port);
			} else {
				_schalte_netz_relay(false, cfg->auto_laden_host, cfg->auto_laden_port);
			}
		}

		void wechsle_auto_ladeleistung() {
			// TODO
		}

		void wechsle_roller_ladeleistung() {
			// TODO
		}
	};
}
