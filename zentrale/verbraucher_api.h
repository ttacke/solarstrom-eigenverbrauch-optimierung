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
		const char* roller_ladestatus_filename = "roller.ladestatus";
		const char* roller_leistung_filename = "roller_leistung.status";
		const char* roller_leistung_log_filename = "roller_leistung.log";

		const char* auto_relay_zustand_seit_filename = "auto_relay.zustand_seit";
		const char* auto_ladestatus_filename = "auto.ladestatus";
		const char* auto_leistung_filename = "auto_leistung.status";
		const char* auto_leistung_log_filename = "auto_leistung.log";

		const char* ueberschuss_leistung_log_filename = "ueberschuss_leistung.log";

		const char* log_filename = "verbraucher_automatisierung.log";

		char temp_log_val[32] = "";

		void _log(char* msg) {
			if(persistenz->open_file_to_append(log_filename)) {
				sprintf(persistenz->buffer, "%d:", timestamp);
				persistenz->print_buffer_to_file();

				sprintf(persistenz->buffer, "%s\n", msg);
				persistenz->print_buffer_to_file();

				persistenz->close_file();
			}
		}

		void _log(char* key, char* msg) {
			if(persistenz->open_file_to_append(log_filename)) {
				sprintf(persistenz->buffer, "%d:", timestamp);
				persistenz->print_buffer_to_file();

				sprintf(persistenz->buffer, "%s>%s\n", key, msg);
				persistenz->print_buffer_to_file();

				persistenz->close_file();
			}
		}

		int _gib_listen_maximum(int* liste) {
			int max = liste[0];
			for(int i = 1; i < 5; i++) {
				if(liste[i] > max) {
					max = liste[i];
				}
			}
			return max;
		}

		int _gib_listen_minimum(int* liste) {
			int min = liste[0];
			for(int i = 1; i < 5; i++) {
				if(liste[i] < min) {
					min = liste[i];
				}
			}
			return min;
		}

		bool _laden_ist_beendet(
			char* log_key,
			int relay_zustand_seit,
			int min_schaltzeit_in_min,
			Local::Verbraucher::Ladestatus& ladestatus,
			bool relay_ist_an,
			int* ladeleistung_log_in_w,
			int benoetigte_ladeleistung_in_w
		) {
			bool schalt_mindestdauer_ist_erreicht = timestamp - relay_zustand_seit >= min_schaltzeit_in_min * 60;
			if(!schalt_mindestdauer_ist_erreicht) {
				_log(log_key, (char*) "_laden_ist_beendet>schaltdauerNichtErreicht");
				return false;
			}

			if(
				(
					ladestatus == Local::Verbraucher::Ladestatus::off
					&& relay_ist_an
				) || (
					ladestatus == Local::Verbraucher::Ladestatus::force
					&& !relay_ist_an
				)
			) {
				_log(log_key, (char*) "_laden_ist_beendet>StatusFehlerKorrigiert");
				return true;
			}
			if(relay_ist_an) {
				if(
					_gib_listen_maximum(ladeleistung_log_in_w)
					<
					(float) benoetigte_ladeleistung_in_w * 0.7
				) {
					_log(log_key, (char*) "_laden_ist_beendet>AusWeilBeendet");
					return true;
				}
			}
			return false;
		}

		float _gib_geforderte_leistung_anhand_der_ladekurve(
			int akku, int x_offset, float y_offset, float min_val, float ladekurven_faktor
		) {
			int x = akku + x_offset;
			float gefordert = 0;
			if(x > 900) {
				gefordert = 1.0 - (0.01 * (x - 900));
			} else if(akku > 350) {
				gefordert = 2.1 - (0.002 * (x - 350));
			} else {
				gefordert = 5.6 - (0.01 * x);
			}
			gefordert += y_offset;
			if(gefordert < min_val) {
				return min_val;
			}
			if(gefordert > 0) {
				gefordert *= ladekurven_faktor;
			}
			return gefordert;
		}

		template<typename F1, typename F2>
		bool _schalte_automatisch(
			char* log_key,
			Local::Verbraucher& verbraucher,
			int relay_zustand_seit,
			int min_schaltzeit_in_min,
			int benoetigte_leistung_in_w,
			bool relay_ist_an,
			int x_offset,
			float y_offset,
			float ladekurven_faktor,
			F1 && log_func,
			F2 && schalt_func
		) {
			bool schalt_mindestdauer_ist_erreicht = timestamp - relay_zustand_seit >= min_schaltzeit_in_min * 60;
			if(!schalt_mindestdauer_ist_erreicht) {
				_log(log_key, (char*) "-automatisch>SchaltdauerNichtErreicht");
				return false;
			}

			int akku = verbraucher.aktueller_akku_ladenstand_in_promille;
			float min_bereitgestellte_leistung
				= (float) _gib_listen_minimum(verbraucher.ueberschuss_log_in_w) / (float) benoetigte_leistung_in_w;
			float geforderte_leistung_fuer_einschalten = _gib_geforderte_leistung_anhand_der_ladekurve(
				akku, x_offset, y_offset, 0, ladekurven_faktor
			);
			float geforderte_leistung_fuer_ausschalten = _gib_geforderte_leistung_anhand_der_ladekurve(
				akku, x_offset + 100, y_offset, -0.1, ladekurven_faktor
			);

			_log(log_key, (char*) "-automatisch>Daten");
			sprintf(
				temp_log_val,
				"IST:%f on:%f off:%f",
				min_bereitgestellte_leistung,
				geforderte_leistung_fuer_einschalten,
				geforderte_leistung_fuer_ausschalten
			);
			_log(log_key, (char*) temp_log_val);

			if(!relay_ist_an) {
				log_func(min_bereitgestellte_leistung, geforderte_leistung_fuer_einschalten);
				if(
					verbraucher.solarerzeugung_ist_aktiv()
					&& min_bereitgestellte_leistung > geforderte_leistung_fuer_einschalten
				) {
					_log(log_key, (char*) "-automatisch>AnWeilGenug");
					schalt_func(true);
					return true;
				}
			} else {
				log_func(min_bereitgestellte_leistung, (geforderte_leistung_fuer_ausschalten - 1.0));
				if(
					!verbraucher.solarerzeugung_ist_aktiv()
					|| min_bereitgestellte_leistung < (geforderte_leistung_fuer_ausschalten - 1.0)
				) {
					_log(log_key, (char*) "-automatisch>AusWeilZuWenig");
					schalt_func(false);
					return true;
				}
			}
			return false;
		}

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
				if(web_client->find_in_buffer((char*) "\"ison\":true")) {
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
				if(web_client->find_in_buffer((char*) "\"ison\":true")) {
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
				if(web_client->find_in_buffer((char*) "\"power\":([0-9]+)")) {
					return atoi(web_client->finding_buffer);
				}
			}
			return 0;
		}

		int _gib_auto_benoetigte_ladeleistung_in_w() {
			int leistung = cfg->auto_benoetigte_leistung_gering_in_w;
			if(persistenz->open_file_to_read(auto_leistung_filename)) {
				while(persistenz->read_next_block_to_buffer()) {
					if(persistenz->find_in_buffer((char*) "([0-9]+)")) {
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

		void _schalte_wasser_relay(bool ein) {
			_schalte_netz_relay(ein, cfg->wasser_relay_host, cfg->wasser_relay_port);
			if(persistenz->open_file_to_overwrite(wasser_relay_zustand_seit_filename)) {
				sprintf(persistenz->buffer, "%d", timestamp);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
		}

		void _schalte_heizung_relay(bool ein) {
			_schalte_netz_relay(ein, cfg->heizung_relay_host, cfg->heizung_relay_port);
			if(persistenz->open_file_to_overwrite(heizung_relay_zustand_seit_filename)) {
				sprintf(persistenz->buffer, "%d", timestamp);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
		}

		int _lese_zustand_seit(const char* filename) {
			int seit = 0;
			if(persistenz->open_file_to_read(filename)) {
				while(persistenz->read_next_block_to_buffer()) {
					if(persistenz->find_in_buffer((char*) "([0-9]+)")) {
						seit = atoi(persistenz->finding_buffer);
					}
				}
				persistenz->close_file();
			}
			return seit;
		}

		int _gib_roller_benoetigte_ladeleistung_in_w() {
			int leistung = cfg->roller_benoetigte_leistung_hoch_in_w;
			if(persistenz->open_file_to_read(roller_leistung_filename)) {
				while(persistenz->read_next_block_to_buffer()) {
					if(persistenz->find_in_buffer((char*) "([0-9]+)")) {
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

		void _lies_verbraucher_log(int* liste, const char* log_filename) {
			for(int i = 0; i < 5; i++) {
				liste[i] = 0;
			}
			if(persistenz->open_file_to_read(log_filename)) {
				while(persistenz->read_next_block_to_buffer()) {
					char suche[16] = "";
					for(int i = 0; i < 5; i++) {
						sprintf(suche, ">%d=([-0-9]+)<", i);
						if(persistenz->find_in_buffer((char*) suche)) {
							liste[i] = atoi(persistenz->finding_buffer);
						}
					}
				}
				persistenz->close_file();
			}
		}

		void _schreibe_verbraucher_log(int* liste, int aktuell, const char* log_filename) {
			for(int i = 1; i < 5; i++) {
				liste[i - 1] = liste[i];
			}
			liste[4] = aktuell;

			if(persistenz->open_file_to_overwrite(log_filename)) {
				for(int i = 0; i < 5; i++) {
					sprintf(persistenz->buffer, ">%d=%d<", i, liste[i]);
					persistenz->print_buffer_to_file();
				}
				persistenz->close_file();
			}
		}

		void _lese_ladestatus(Local::Verbraucher::Ladestatus& ladestatus, const char* filename) {
			ladestatus = Local::Verbraucher::Ladestatus::off;
			if(persistenz->open_file_to_read(filename)) {
				while(persistenz->read_next_block_to_buffer()) {
					if(persistenz->find_in_buffer((char*) "([a-z]+)")) {
						if(strcmp(persistenz->finding_buffer, "solar") == 0) {
							ladestatus = Local::Verbraucher::Ladestatus::solar;
						} else if(strcmp(persistenz->finding_buffer, "force") == 0) {
							ladestatus = Local::Verbraucher::Ladestatus::force;
						}
						return;
					}
				}
				persistenz->close_file();
			}
		}

	public:
		VerbraucherAPI(
			Local::Config& cfg,
			Local::WebClient& web_client,
			Local::Persistenz& persistenz,
			int timestamp
		): BaseAPI(cfg, web_client), persistenz(&persistenz), timestamp(timestamp) {
		}

		void daten_holen_und_einsetzen(
			Local::Verbraucher& verbraucher,
			Local::ElektroAnlage& elektroanlage,
			Local::Wetter wetter
		) {
			_ermittle_relay_zustaende(verbraucher);

			verbraucher.aktuelle_auto_ladeleistung_in_w = round(
				(float) (elektroanlage.l3_strom_ma + elektroanlage.l3_solarstrom_ma) / 1000 * 230
			);
			_lies_verbraucher_log(verbraucher.auto_ladeleistung_log_in_w, auto_leistung_log_filename);
			_schreibe_verbraucher_log(verbraucher.auto_ladeleistung_log_in_w, verbraucher.aktuelle_auto_ladeleistung_in_w, auto_leistung_log_filename);
			verbraucher.auto_benoetigte_ladeleistung_in_w = _gib_auto_benoetigte_ladeleistung_in_w();

			verbraucher.aktuelle_roller_ladeleistung_in_w = _gib_aktuelle_shellyplug_leistung(cfg->roller_relay_host, cfg->roller_relay_port);
			_lies_verbraucher_log(verbraucher.roller_ladeleistung_log_in_w, roller_leistung_log_filename);
			_schreibe_verbraucher_log(verbraucher.roller_ladeleistung_log_in_w, verbraucher.aktuelle_roller_ladeleistung_in_w, roller_leistung_log_filename);
			verbraucher.roller_benoetigte_ladeleistung_in_w = _gib_roller_benoetigte_ladeleistung_in_w();

			verbraucher.aktueller_ueberschuss_in_w = elektroanlage.gib_ueberschuss_in_w();
			_lies_verbraucher_log(verbraucher.ueberschuss_log_in_w, ueberschuss_leistung_log_filename);
			_schreibe_verbraucher_log(verbraucher.ueberschuss_log_in_w, verbraucher.aktueller_ueberschuss_in_w, ueberschuss_leistung_log_filename);

			verbraucher.aktueller_akku_ladenstand_in_promille = elektroanlage.solarakku_ladestand_in_promille;
			verbraucher.solarerzeugung_in_w = elektroanlage.solarerzeugung_in_w;
			verbraucher.zeitpunkt_sonnenuntergang = wetter.zeitpunkt_sonnenuntergang;

			_lese_ladestatus(verbraucher.auto_ladestatus, auto_ladestatus_filename);
			_lese_ladestatus(verbraucher.roller_ladestatus, roller_ladestatus_filename);
		}

		void fuehre_schaltautomat_aus(Local::Verbraucher& verbraucher) {
			if(_laden_ist_beendet(
				(char*) "roller",
				verbraucher.roller_relay_zustand_seit,
				cfg->roller_min_schaltzeit_in_min,
				verbraucher.roller_ladestatus,
				verbraucher.roller_relay_ist_an,
				verbraucher.roller_ladeleistung_log_in_w,
				verbraucher.roller_benoetigte_ladeleistung_in_w
			)) {
				setze_roller_ladestatus(Local::Verbraucher::Ladestatus::off);
				return;
			}

			if(_laden_ist_beendet(
				(char*) "auto",
				verbraucher.auto_relay_zustand_seit,
				cfg->auto_min_schaltzeit_in_min,
				verbraucher.auto_ladestatus,
				verbraucher.auto_relay_ist_an,
				verbraucher.auto_ladeleistung_log_in_w,
				verbraucher.auto_benoetigte_ladeleistung_in_w
			)) {
				setze_auto_ladestatus(Local::Verbraucher::Ladestatus::off);
				return;
			}

			int karenzzeit = (cfg->schaltautomat_schalt_karenzzeit_in_min * 60);
			if(
				verbraucher.auto_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.roller_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.wasser_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.heizung_relay_zustand_seit >= timestamp - karenzzeit
			) {
				_log((char*) "SchaltKarenzzeit");
				return;
			}

			float ladekurven_faktor = _ermittle_ladekurvenfaktor(verbraucher);
			float ueberladen_ladekurven_faktor = _ermittle_ueberladen_ladekurvenfaktor(verbraucher, ladekurven_faktor);

			if(
				verbraucher.auto_ladestatus == Local::Verbraucher::Ladestatus::solar
				&& _schalte_automatisch(
					(char*) "auto",
					verbraucher,
					verbraucher.auto_relay_zustand_seit,
					cfg->auto_min_schaltzeit_in_min,
					verbraucher.auto_benoetigte_ladeleistung_in_w,
					verbraucher.auto_relay_ist_an,
					-50, -1.0, ladekurven_faktor,
					[&](float ist, float soll) {
						verbraucher.auto_leistung_ist = ist;
						verbraucher.auto_leistung_soll = soll;
					},
					[&](bool ein) { _schalte_auto_relay(ein); }
				)
			) {
				return;
			}
			if(
				verbraucher.roller_ladestatus == Local::Verbraucher::Ladestatus::solar
				&& _schalte_automatisch(
					(char*) "roller",
					verbraucher,
					verbraucher.roller_relay_zustand_seit,
					cfg->roller_min_schaltzeit_in_min,
					verbraucher.roller_benoetigte_ladeleistung_in_w,
					verbraucher.roller_relay_ist_an,
					-50, -1.0, ladekurven_faktor,
					[&](float ist, float soll) {
						verbraucher.roller_leistung_ist = ist;
						verbraucher.roller_leistung_soll = soll;
					},
					[&](bool ein) { _schalte_roller_relay(ein); }
				)
			) {
				return;
			}
			// TODO deaktiviert, weil so nicht sinnvoll
//			if(_schalte_automatisch(
//				(char*) "wasser",
//				verbraucher,
//				verbraucher.wasser_relay_zustand_seit,
//				cfg->wasser_min_schaltzeit_in_min,
//				cfg->wasser_benoetigte_leistung_in_w,
//				verbraucher.wasser_relay_ist_an,
//				0, 0, ueberladen_ladekurven_faktor,
//				[&](float ist, float soll) {
//					verbraucher.wasser_leistung_ist = ist;
//					verbraucher.wasser_leistung_soll = soll;
//				},
//				[&](bool ein) { _schalte_wasser_relay(ein); }
//			)) {
//				return;
//			}


			// TODO
/*

Ueberschuss-Log
- 30 einträge lang machen, weil die Schwankungen sonst zu groß sind
- Nicht min/max, sondern median nutzen!




			 Ueberladen von der Vorhersage / Überschuss abhängig machen
			 nur wenn mehr als 3?? Balken auch in der Zukunft sind, dann starten
			 sonst nur bei >80%

			 Laden nur als 2Stufige Linie? Oder nur eine Stufe?
			 < 35% nie
			 < 80% immer (wenn+ und sonnenuntergang > Xh und auch mehr weiterhin Strahlungsvorhersage nicht unter Real Null sinkt)
*/

			// TODO deaktiviert, weil so nicht sinnvoll
//			if(_schalte_automatisch(
//				(char*) "heizung",
//				verbraucher,
//				verbraucher.heizung_relay_zustand_seit,
//				cfg->heizung_min_schaltzeit_in_min,
//				cfg->heizung_benoetigte_leistung_in_w,
//				verbraucher.heizung_relay_ist_an,
//				0, 0, ueberladen_ladekurven_faktor,
//				[&](float ist, float soll) {
//					verbraucher.heizung_leistung_ist = ist;
//					verbraucher.heizung_leistung_soll = soll;
//				},
//				[&](bool ein) { _schalte_heizung_relay(ein); }
//			)) {
//				return;
//			}
		}

		float _ermittle_ladekurvenfaktor(Local::Verbraucher& verbraucher) {
			float ladekurven_faktor = 1.0;
			if(verbraucher.zeitpunkt_sonnenuntergang > 0) {
				int sonnenuntergang_abstand_in_s = verbraucher.zeitpunkt_sonnenuntergang - timestamp;
				if(sonnenuntergang_abstand_in_s < 0) {
					sonnenuntergang_abstand_in_s = 0;
				}
				if(sonnenuntergang_abstand_in_s < 1 * 3600) {// 1.5 .. 4
					ladekurven_faktor *= 1.5 + (2.5 / (1 * 3600)) * (1 * 3600 - sonnenuntergang_abstand_in_s);
				} else if(sonnenuntergang_abstand_in_s < 2 * 3600) {// 0.5 .. 1.5
					ladekurven_faktor *= 0.5 + (1.0 / (1 * 3600)) * (2 * 3600 - sonnenuntergang_abstand_in_s);
				} else if(sonnenuntergang_abstand_in_s < 3 * 3600) {// 0 .. 0.5
					ladekurven_faktor += 0 + (0.5 / (1 * 3600)) * (3 * 3600 - sonnenuntergang_abstand_in_s);
				}
			}
			return ladekurven_faktor;
		}

		float _ermittle_ueberladen_ladekurvenfaktor(Local::Verbraucher& verbraucher, float ladekurven_faktor) {
			float ueberladen_ladekurven_faktor = ladekurven_faktor;
			if(
				verbraucher.auto_ladestatus == Local::Verbraucher::Ladestatus::solar
				&& !verbraucher.auto_relay_ist_an
			) {
				ueberladen_ladekurven_faktor += 3.0;
			}
			if(
				verbraucher.roller_ladestatus == Local::Verbraucher::Ladestatus::solar
				&& !verbraucher.roller_relay_ist_an
			) {
				ueberladen_ladekurven_faktor += 1.5;
			}
			return ueberladen_ladekurven_faktor;
		}

		void setze_roller_ladestatus(Local::Verbraucher::Ladestatus status) {
			char stat[6];
			if(status == Local::Verbraucher::Ladestatus::force) {
				_schalte_roller_relay(true);
				strcpy(stat, "force");
				_log((char*) "setze_roller_ladestatus>force");
			} else if(status == Local::Verbraucher::Ladestatus::solar) {
				_schalte_roller_relay(false);
				strcpy(stat, "solar");
				_log((char*) "setze_roller_ladestatus>solar");
			} else {
				_schalte_roller_relay(false);
				strcpy(stat, "off");
				_log((char*) "setze_roller_ladestatus>off");
			}
			if(persistenz->open_file_to_overwrite(roller_ladestatus_filename)) {
				sprintf(persistenz->buffer, "%s", stat);
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
				_log((char*) "setze_auto_ladestatus>force");
			} else if(status == Local::Verbraucher::Ladestatus::solar) {
				_schalte_auto_relay(false);
				strcpy(stat, "solar");
				_log((char*) "setze_auto_ladestatus>solar");
			} else {
				_schalte_auto_relay(false);
				strcpy(stat, "off");
				_log((char*) "setze_auto_ladestatus>off");
			}
			if(persistenz->open_file_to_overwrite(auto_ladestatus_filename)) {
				sprintf(persistenz->buffer, "%s", stat);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
			if(persistenz->open_file_to_overwrite(auto_leistung_log_filename)) {
				persistenz->close_file();
			}
		}

		void wechsle_auto_ladeleistung() {
			_log((char*) "wechsle_auto_ladeleistung");
			int auto_benoetigte_ladeleistung_in_w = _gib_auto_benoetigte_ladeleistung_in_w();
			if(auto_benoetigte_ladeleistung_in_w == cfg->auto_benoetigte_leistung_hoch_in_w) {
				auto_benoetigte_ladeleistung_in_w = cfg->auto_benoetigte_leistung_gering_in_w;
			} else {
				auto_benoetigte_ladeleistung_in_w = cfg->auto_benoetigte_leistung_hoch_in_w;
			}
			if(persistenz->open_file_to_overwrite(auto_leistung_filename)) {
				sprintf(persistenz->buffer, "%d", auto_benoetigte_ladeleistung_in_w);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
		}

		void wechsle_roller_ladeleistung() {
			_log((char*) "wechsle_roller_ladeleistung");
			int roller_benoetigte_ladeleistung_in_w = _gib_roller_benoetigte_ladeleistung_in_w();
			if(roller_benoetigte_ladeleistung_in_w == cfg->roller_benoetigte_leistung_hoch_in_w) {
				roller_benoetigte_ladeleistung_in_w = cfg->roller_benoetigte_leistung_gering_in_w;
			} else {
				roller_benoetigte_ladeleistung_in_w = cfg->roller_benoetigte_leistung_hoch_in_w;
			}
			if(persistenz->open_file_to_overwrite(roller_leistung_filename)) {
				sprintf(persistenz->buffer, "%d", roller_benoetigte_ladeleistung_in_w);
				persistenz->print_buffer_to_file();
				persistenz->close_file();
			}
		}
	};
}
