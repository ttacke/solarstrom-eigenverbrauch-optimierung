#pragma once
#include "base_api.h"

namespace Local {
	class RollerNetzRelayAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		bool _ist_an(const char* host, int port) {
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

		bool _es_wird_geladen(const char* host, int port) {
			web_client->send_http_get_request(
				host,
				port,
				"/status"
			);
			while(web_client->read_next_block_to_buffer()) {
				if(web_client->find_in_content((char*) "\"power\":([0-9]+)")) {
					int leistung = atoi(web_client->finding_buffer);
					if(leistung > 50) {
						return true;
					}
					return false;
				}
			}
			return false;
		}

		// TODO: "temperature":23.62[,}]  --> loggen? Aber das ist im Sommer draussen :/

	public:
		bool ist_force_aktiv() {
			return _ist_an(cfg->roller_laden_host, cfg->roller_laden_port);
		}

		bool ist_solar_aktiv() {
			return false;
		}

		void schalte(bool ein) {
			web_client->send_http_get_request(
				cfg->roller_laden_host,
				cfg->roller_laden_port,
				(ein ? "/relay/0?turn=on" : "/relay/0?turn=off")
			);
		}

		int gib_aktuelle_ladeleistung_in_w() {
			return 840;
		}
	};
}
