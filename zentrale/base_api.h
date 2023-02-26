#pragma once
#include "web_client.h"
#include "config.h"
#include <Regexp.h>

namespace Local {
	class BaseAPI {

	protected:
		Local::WebClient* web_client;
		Local::Config* cfg;

		bool _ist_an(const char* host, int port) {
			web_client->send_http_get_request(
				host,
				port,
				"/relay"
			);
			while(web_client->read_next_block_to_buffer()) {
				if(web_client->find_in_content((char*) "true")) {
					return true;
				}
			}
			return false;
		}

		void _schalte(const char* host, int port, bool ein) {
			web_client->send_http_get_request(
				host,
				port,
				(ein ? "/relay?set_relay=true" : "/relay?set_relay=false")
			);
		}

	public:
		BaseAPI(Local::Config& cfg, Local::WebClient& web_client): cfg(&cfg), web_client(&web_client) {
		}

		virtual void schalte(bool ein) {
		}

		void fuehre_aus(const char* val, int now_timestamp) {
			// Implementieren
			if(strcmp(val, "force") == 0) {
				schalte(true);
			} else if(strcmp(val, "solar") == 0) {
				schalte(false);
			} else if(strcmp(val, "off") == 0) {
				schalte(false);
			} else if(strcmp(val, "change_power") == 0) {
				// TODO
			}
		}

		void heartbeat(int now_timestamp) {
			// TODO wird 1x die Minute gestartet
		}
	};
}
