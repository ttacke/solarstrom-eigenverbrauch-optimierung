#pragma once
#include "base_api.h"

namespace Local::Api {
	class VerbraucherAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		Local::Service::FileReader* file_reader;
		Local::Service::FileWriter& file_writer;
		int shelly_roller_cache_timestamp = 0;
		bool shelly_roller_cache_ison = false;
		int shelly_roller_cache_power = 0;
		int roller_benoetigte_ladeleistung_in_w_cache = 0;
		bool ladeverhalten_wintermodus_cache = false;
		const char* ladeverhalten_wintermodus_filename = "ladeverhalten_wintermodus.status";
		bool frueh_leeren_lief_heute = true;
		bool frueh_leeren_ist_aktiv = false;

		const char* heizung_relay_zustand_seit_filename = "heizung_relay.status";

		const char* wasser_relay_zustand_seit_filename = "wasser_relay.status";

		const char* roller_relay_zustand_seit_filename = "roller_relay.zustand_seit";
		const char* roller_ladestatus_filename = "roller.ladestatus";
		const char* roller_leistung_filename = "roller_leistung.status";
		const char* roller_leistung_log_filename = "roller_leistung.log";

		const char* auto_relay_zustand_seit_filename = "auto_relay.zustand_seit";
		const char* auto_ladestatus_filename = "auto.ladestatus";
		const char* auto_leistung_log_filename = "auto_leistung.log";

		const char* verbrauch_leistung_log_filename = "verbrauch_leistung.log";
		const char* erzeugung_leistung_log_filename = "erzeugung_leistung.log";

		const char* automatisierung_log_filename_template = "verbraucher_automatisierung-%4d-%02d.log";
		const char* frueh_leeren_status_filename_template = "frueh_laden_%s.status";

		void _lese_frueh_leeren_status(char* key) {
			char filename[32];
			sprintf(filename, frueh_leeren_status_filename_template, key);
			frueh_leeren_lief_heute = false;
			frueh_leeren_ist_aktiv = false;
			if(file_reader->open_file_to_read(filename)) {
				while(file_reader->read_next_block_to_buffer()) {
					if(file_reader->find_in_buffer((char*) "([0-9-]+);")) {
						char heute[16] = "";
						sprintf(heute, "%4d-%02d-%02d", year(timestamp), month(timestamp), day(timestamp));
						if(strcmp(file_reader->finding_buffer, heute) == 0) {
							frueh_leeren_lief_heute = true;
						}
					}
					if(file_reader->find_in_buffer((char*) "(an)")) {
						frueh_leeren_ist_aktiv = true;
					}
				}
				file_reader->close_file();
			}
		}

		void _schreibe_frueh_leeren_status(char* key, bool ist_aktiv) {
			char filename[32];
			sprintf(filename, frueh_leeren_status_filename_template, key);
			if(file_writer.open_file_to_overwrite(filename)) {
				file_writer.write_formated(
					"%4d-%02d-%02d;%s", year(timestamp), month(timestamp), day(timestamp),
					ist_aktiv ? "an" : "aus"
				);
				file_writer.close_file();
			}
		}

		void _log(char* msg) {
			char filename[32];
			sprintf(filename, automatisierung_log_filename_template, year(timestamp), month(timestamp));
			if(file_writer.open_file_to_append(filename)) {
				file_writer.write_formated("%d:%s\n", timestamp, msg);
				file_writer.close_file();
			}
		}

		void _log(char* key, char* msg) {
			char filename[32];
			sprintf(filename, automatisierung_log_filename_template, year(timestamp), month(timestamp));
			if(file_writer.open_file_to_append(filename)) {
				file_writer.write_formated("%d:%s>%s\n", timestamp, key, msg);
				file_writer.close_file();
			}
		}

		float _gib_min_bereitgestellte_leistung(Local::Model::Verbraucher& verbraucher, int benoetigte_leistung_in_w) {
			return
				(float) verbraucher.gib_beruhigten_ueberschuss_in_w()
				/
				(float) benoetigte_leistung_in_w
			;
		}

		float _ermittle_solarladen_einschaltschwelle(
			int aktueller_akku_ladenstand_in_promille,
			int start_ladebereich
		) {
			if(aktueller_akku_ladenstand_in_promille < start_ladebereich) {
				return 99999;
			}
			float einschaltschwelle = 1.1 - (0.004 * (aktueller_akku_ladenstand_in_promille - start_ladebereich));
			if(einschaltschwelle <= 0.01) {
				einschaltschwelle = 0.01;
			}
			return einschaltschwelle;
		}

		bool _ausschalten_wegen_lastgrenzen(Local::Model::Verbraucher& verbraucher) {
			return _einschalten_wegen_lastgrenzen_verboten(verbraucher, 0);
		}

		bool _einschalten_wegen_lastgrenzen_verboten(
			Local::Model::Verbraucher& verbraucher,
			int benoetigte_leistung_in_w
		) {
			int reserve = 0;
			if(benoetigte_leistung_in_w == 0) {
				reserve = cfg->einschaltreserve_in_w;
			}
			if(
				verbraucher.aktueller_verbrauch_in_w + benoetigte_leistung_in_w
				>
				cfg->maximale_wechselrichterleistung_in_w - reserve
			) {
				_log((char*) "Lastschutz>maxWR");
				return true;
			}
			if(
				verbraucher.ersatzstrom_ist_aktiv
				&&
				verbraucher.aktueller_verbrauch_in_w + benoetigte_leistung_in_w
				>
				((float) cfg->maximale_wechselrichterleistung_in_w * 0.8) - reserve
			) {
				_log((char*) "Lastschutz>ersatz");
				return true;
			}

			if(
				verbraucher.netzbezug_in_w + benoetigte_leistung_in_w
				>=
				cfg->maximaler_netzbezug_ausschaltgrenze_in_w - reserve
			) {
				_log((char*) "Lastschutz>maxBezug");
				return true;
			}
			return false;
		}

		template<typename F1, typename F2>
		bool _behandle_solar_laden(
			char* log_key,
			Local::Model::Verbraucher& verbraucher,
			int relay_zustand_seit,
			int min_schaltzeit_in_min,
			int benoetigte_leistung_in_w,
			bool relay_ist_an,
			int akku_zielladestand_in_promille,
			F1 && schalt_func,
			F2 && lastschutz_an_func
		) {
			bool schalt_mindestdauer_ist_erreicht = timestamp - relay_zustand_seit >= min_schaltzeit_in_min * 60;
			if(!schalt_mindestdauer_ist_erreicht) {
				_log(log_key, (char*) "-solar>SchaltdauerNichtErreicht");
				return false;
			}

			float min_bereitgestellte_leistung = _gib_min_bereitgestellte_leistung(
				verbraucher, benoetigte_leistung_in_w
			);
			float start_puffer_in_wh = ((float) min_schaltzeit_in_min / 60) * (float) benoetigte_leistung_in_w;
			int start_puffer_in_promille = round((float) start_puffer_in_wh / (cfg->akku_groesse_in_wh / 1000));
			if(!relay_ist_an) {
				akku_zielladestand_in_promille += start_puffer_in_promille;
			}
			bool akku_erreicht_zielladestand = verbraucher.akku_erreicht_ladestand_in_promille(
				akku_zielladestand_in_promille
			);
			bool akku_unterschreitet_minimalladestand = verbraucher.akku_unterschreitet_ladestand_in_promille(
				cfg->minimaler_akku_ladestand
			);
			float nicht_laden_unter_akkuladestand = 400;
			float einschaltschwelle = _ermittle_solarladen_einschaltschwelle(
				verbraucher.aktueller_akku_ladenstand_in_promille,
				nicht_laden_unter_akkuladestand
			);

			int sonnenuntergang_abstand_in_s = 0;
			if(verbraucher.zeitpunkt_sonnenuntergang > 0) {
				sonnenuntergang_abstand_in_s = verbraucher.zeitpunkt_sonnenuntergang - timestamp;
			}

			_lese_frueh_leeren_status(log_key);
			if(
				!frueh_leeren_lief_heute
				&& !frueh_leeren_ist_aktiv
				&& !relay_ist_an
				&& hour(timestamp) == cfg->frueh_leeren_starte_in_stunde_utc
				&& akku_erreicht_zielladestand
				&& !akku_unterschreitet_minimalladestand
				&&
					verbraucher.aktueller_akku_ladenstand_in_promille
					>=
					cfg->minimaler_akku_ladestand + start_puffer_in_promille
			) {
				if(_einschalten_wegen_lastgrenzen_verboten(verbraucher, benoetigte_leistung_in_w)) {
					lastschutz_an_func();
				} else {
					_log(log_key, (char*) "-solar>FruehLeerenAn");
					_schreibe_frueh_leeren_status(log_key, true);
					schalt_func(true);
					return true;
				}
			}

			if(frueh_leeren_ist_aktiv) {
				if(
					relay_ist_an
					&& (
						!akku_erreicht_zielladestand
						|| akku_unterschreitet_minimalladestand
						||
							verbraucher.aktueller_akku_ladenstand_in_promille
							<
							cfg->minimaler_akku_ladestand
					)
				) {
					_log(log_key, (char*) "-solar>FruehLeerenAus");
					_schreibe_frueh_leeren_status(log_key, false);
					schalt_func(false);
					return true;
				}
				return false;// Weitere Steuerung in der Zeit unterbinden
			}

			if(
				!relay_ist_an
				&& verbraucher.solarerzeugung_ist_aktiv()
				&& sonnenuntergang_abstand_in_s > 0.5 * 3600
				&& akku_erreicht_zielladestand
				&& !akku_unterschreitet_minimalladestand
				&& min_bereitgestellte_leistung > einschaltschwelle
			) {
				if(_einschalten_wegen_lastgrenzen_verboten(verbraucher, benoetigte_leistung_in_w)) {
					lastschutz_an_func();
				} else {
					_log(log_key, (char*) "-solar>AnWeilGenug");
					schalt_func(true);
					return true;
				}
			}
			if(relay_ist_an) {
				if(
					!akku_erreicht_zielladestand
					|| akku_unterschreitet_minimalladestand
					|| !verbraucher.solarerzeugung_ist_aktiv()
				) {
					_log(log_key, (char*) "-aus>AusWegenZielladestand");
					schalt_func(false);
					return true;
				}
				if(verbraucher.aktueller_akku_ladenstand_in_promille < nicht_laden_unter_akkuladestand) {
					_log(log_key, (char*) "-aus>AusWegenZuWenigAkku");
					schalt_func(false);
					return true;
				}
			}
			return false;
		}

		void _ermittle_relay_zustaende(Local::Model::Verbraucher& verbraucher) {
			verbraucher.heizung_relay_ist_an = _netz_relay_ist_an(cfg->heizung_relay_host, cfg->heizung_relay_port);
			verbraucher.heizung_relay_zustand_seit = _lese_zustand_seit(heizung_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher.wasser_relay_ist_an = _netz_relay_ist_an(cfg->wasser_relay_host, cfg->wasser_relay_port);
			verbraucher.wasser_relay_zustand_seit = _lese_zustand_seit(wasser_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher.auto_relay_ist_an = _netz_relay_ist_an(cfg->auto_relay_host, cfg->auto_relay_port);
			verbraucher.auto_relay_zustand_seit = _lese_zustand_seit(auto_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher.roller_relay_ist_an = shelly_roller_cache_ison;
			verbraucher.roller_relay_zustand_seit = _lese_zustand_seit(roller_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
		}

		bool _shelly_plug_ist_an(const char* host, int port) {
			web_reader->send_http_get_request(host, port, "/status");
			while(web_reader->read_next_block_to_buffer()) {
				if(web_reader->find_in_buffer((char*) "\"ison\":true")) {
					return true;
				}
			}
			return false;
		}

		bool _netz_relay_ist_an(const char* host, int port) {
			web_reader->send_http_get_request(host, port, "/relay");
			while(web_reader->read_next_block_to_buffer()) {
				if(web_reader->find_in_buffer((char*) "\"ison\":true")) {
					return true;
				}
			}
			return false;
		}

		void _schalte_netz_relay(bool ein, const char* host, int port) {
			web_reader->send_http_get_request(
				host,
				port,
				(ein ? "/relay?set_relay=true" : "/relay?set_relay=false")
			);
		}

		void _schalte_shellyplug(bool ein, const char* host, int port, int timeout) {
			web_reader->send_http_get_request(
				host,
				port,
				(ein ? "/relay/0?turn=on" : "/relay/0?turn=off"),
				timeout
			);
		}

		void _schalte_roller_relay(bool ein) {
			_schalte_shellyplug(
				ein, cfg->roller_relay_host, cfg->roller_relay_port,
				web_reader->default_timeout_in_hundertstel_s
			);
			if(roller_benoetigte_ladeleistung_in_w_cache == cfg->roller_benoetigte_leistung_gering_in_w) {
				_schalte_shellyplug(
					ein, cfg->roller_aussen_relay_host, cfg->roller_aussen_relay_port,
					web_reader->kurzer_timeout_in_hundertstel_s
				);
			}
			if(file_writer.open_file_to_overwrite(roller_relay_zustand_seit_filename)) {
				file_writer.write_formated("%d", timestamp);
				file_writer.close_file();
			}
		}

		void _schalte_auto_relay(bool ein) {
			_schalte_netz_relay(ein, cfg->auto_relay_host, cfg->auto_relay_port);
			if(file_writer.open_file_to_overwrite(auto_relay_zustand_seit_filename)) {
				file_writer.write_formated("%d", timestamp);
				file_writer.close_file();
			}
		}

		void _schalte_wasser_relay(bool ein) {
			_schalte_netz_relay(ein, cfg->wasser_relay_host, cfg->wasser_relay_port);
			if(file_writer.open_file_to_overwrite(wasser_relay_zustand_seit_filename)) {
				file_writer.write_formated("%d", timestamp);
				file_writer.close_file();
			}
		}

		void _schalte_heizung_relay(bool ein) {
			_schalte_netz_relay(ein, cfg->heizung_relay_host, cfg->heizung_relay_port);
			if(file_writer.open_file_to_overwrite(heizung_relay_zustand_seit_filename)) {
				file_writer.write_formated("%d", timestamp);
				file_writer.close_file();
			}
		}

		int _lese_zustand_seit(const char* filename) {
			int seit = 0;
			if(file_reader->open_file_to_read(filename)) {
				while(file_reader->read_next_block_to_buffer()) {
					if(file_reader->find_in_buffer((char*) "([0-9]+)")) {
						seit = atoi(file_reader->finding_buffer);
					}
				}
				file_reader->close_file();
			}
			return seit;
		}

		int _gib_roller_benoetigte_ladeleistung_in_w() {
			int leistung = cfg->roller_benoetigte_leistung_hoch_in_w;
			if(file_reader->open_file_to_read(roller_leistung_filename)) {
				while(file_reader->read_next_block_to_buffer()) {
					if(file_reader->find_in_buffer((char*) "([0-9]+)")) {
						int i = atoi(file_reader->finding_buffer);
						if(i > 0) {
							leistung = i;
						}
					}
				}
				file_reader->close_file();
			}
			return leistung;
		}

		void _load_ladeverhalten_wintermodus_cache() {
			ladeverhalten_wintermodus_cache = false;
			if(file_reader->open_file_to_read(ladeverhalten_wintermodus_filename)) {
				while(file_reader->read_next_block_to_buffer()) {
					if(file_reader->find_in_buffer((char*) "([0-9]+)")) {
						int i = atoi(file_reader->finding_buffer);
						if(i == 1) {
							ladeverhalten_wintermodus_cache = true;
						}
						break;
					}
				}
				file_reader->close_file();
			}
		}

		void _lies_verbraucher_log(int* liste, const char* log_filename, int length) {
			for(int i = 0; i < length; i++) {
				liste[i] = 0;
			}
			if(file_reader->open_file_to_read(log_filename)) {
				int counter = 0;
				while(file_reader->read_next_block_to_buffer()) {
					counter++;
					char suche[32] = "";
					for(int i = 0; i < length; i++) {
						sprintf(suche, ">%d=([-0-9]+)<", i);
						if(file_reader->find_in_buffer((char*) suche)) {
							liste[i] = atoi(file_reader->finding_buffer);
						}
					}
					if(counter > 100) {
						break;
					}
				}
				file_reader->close_file();
				if(counter > 100) {
					Serial.println("Errors in file. Try to remove it...");
					Serial.println(log_filename);
					if(file_writer.delete_file(log_filename)) {
						Serial.println("ok");
					} else {
						Serial.println("Failed! Cant delete it");
					}
				}
			}
		}

		void _schreibe_verbraucher_log(
			int* liste, int aktuell, const char* log_filename, int length
		) {
			for(int i = 1; i < length; i++) {
				liste[i - 1] = liste[i];
			}
			liste[length - 1] = aktuell;

			if(file_writer.open_file_to_overwrite(log_filename)) {
				for(int i = 0; i < length; i++) {
					file_writer.write_formated(">%d=%d<", i, liste[i]);
				}
				file_writer.close_file();
			}
		}

		int _lese_ladestatus(Local::Model::Verbraucher::Ladestatus& ladestatus, const char* filename) {
			ladestatus = Local::Model::Verbraucher::Ladestatus::solar;
			if(file_reader->open_file_to_read(filename)) {
				while(file_reader->read_next_block_to_buffer()) {
					if(file_reader->find_in_buffer((char*) "([a-z]+),")) {
						if(strcmp(file_reader->finding_buffer, "force") == 0) {
							ladestatus = Local::Model::Verbraucher::Ladestatus::force;
							if(file_reader->find_in_buffer((char*) ",([0-9]+)")) {
								return atoi(file_reader->finding_buffer);
							}
						}
						return 0;
					}
				}
				file_reader->close_file();
			}
			return 0;
		}

		void _fuelle_akkuladestands_vorhersage(
			Local::Model::Verbraucher& verbraucher,
			Local::Model::Wetter wetter
		) {
			int akkuzuwachs_unmoeglich_weil_vergangenheit = 0;
			int zeit_unterschied = timestamp - wetter.stundenvorhersage_startzeitpunkt;
			if(zeit_unterschied < -15 * 60) {
				// DoNothing
			} else if(zeit_unterschied < 0) {
				akkuzuwachs_unmoeglich_weil_vergangenheit = 1;
			} else if(zeit_unterschied > 15 * 60) {
				akkuzuwachs_unmoeglich_weil_vergangenheit = 3;
			} else {
				akkuzuwachs_unmoeglich_weil_vergangenheit = 2;
			}

			int akku_ladestand_in_promille = verbraucher.aktueller_akku_ladenstand_in_promille;
			for(int i = 0; i < 12; i++) {
				int akku_veraenderung_in_promille = round(
					(
						(float) wetter.stundenvorhersage_solarstrahlung_liste[i]
						*
						cfg->solarstrahlungs_vorhersage_umrechnungsfaktor
						- cfg->grundverbrauch_in_w_pro_h
					) / (
						(float) cfg->akku_groesse_in_wh / 1000
					)
				);
				for(int ii = 0; ii < 4; ii++) {
					int index = i * 4 + ii;
					if(index > akkuzuwachs_unmoeglich_weil_vergangenheit) {
						akku_ladestand_in_promille += akku_veraenderung_in_promille / 4;
					}
					if(
						wetter.stundenvorhersage_solarstrahlung_liste[i] < 30
						&& akku_ladestand_in_promille > 1000
					) {// Akku ohne Sonnenschein = Max SOC
						akku_ladestand_in_promille = 1000;
					}
					if(akku_ladestand_in_promille < 50) {// Min SOC
						akku_ladestand_in_promille = 50;
					}
					verbraucher.akku_ladestandsvorhersage_in_promille[index] = akku_ladestand_in_promille;
				}
			}
		}

		void _load_shelly_roller_cache() {
			roller_benoetigte_ladeleistung_in_w_cache = _gib_roller_benoetigte_ladeleistung_in_w();
			bool erfolgreich = false;
			if(roller_benoetigte_ladeleistung_in_w_cache == cfg->roller_benoetigte_leistung_gering_in_w) {
				erfolgreich = web_reader->send_http_get_request(
					cfg->roller_aussen_relay_host,
					cfg->roller_aussen_relay_port,
					"/status"
				);
				if(erfolgreich){
					erfolgreich = _read_shelly_roller_content();
					if(!erfolgreich) {
						Serial.println("Abfrage des Roller-Aussen-Shellys fehlgeschlagen");
					}
				}
			}
			if(!erfolgreich) {// Internen immer abfragen, damit min. ein Datensatz da ist
				web_reader->send_http_get_request(
					cfg->roller_relay_host,
					cfg->roller_relay_port,
					"/status"
				);
				_read_shelly_roller_content();
			}
		}

		bool _read_shelly_roller_content() {
			shelly_roller_cache_timestamp = 0;
			shelly_roller_cache_ison = false;
			shelly_roller_cache_power = 0;
			while(web_reader->read_next_block_to_buffer()) {
				if(web_reader->find_in_buffer((char*) "\"unixtime\":([0-9]+)[^0-9]")) {
					shelly_roller_cache_timestamp = atoi(web_reader->finding_buffer);
				}
				if(web_reader->find_in_buffer((char*) "\"ison\":true")) {
					shelly_roller_cache_ison = true;
				}
				if(web_reader->find_in_buffer((char*) "\"power\":([0-9]+)[^0-9]")) {
					shelly_roller_cache_power = atoi(web_reader->finding_buffer);
				}
			}
			if(shelly_roller_cache_timestamp != 0) {
				return true;
			}
			return false;
		}

		bool _winterladen_ist_aktiv() {
			if(ladeverhalten_wintermodus_cache) {
				int current_hour = hour(timestamp);
				for(int i = 0; i < 12; i++) {
					if(current_hour == cfg->winterladen_zwangspausen_utc[i]) {
						return false;
					}
				}
				return true;
			}
			return false;
		}

		template<typename F1, typename F2, typename F3>
		bool _behandle_force_laden(
			char* log_key,
			Local::Model::Verbraucher& verbraucher,
			int relay_zustand_seit,
			int ladestatus_seit,
			int min_schaltzeit_in_min,
			int benoetigte_leistung_in_w,
			bool relay_ist_an,
			F1 && schalt_func,
			F2 && umschalten_auf_solarladen_func,
			F3 && lastschutz_an_func
		) {
			bool schalt_mindestdauer_ist_erreicht = timestamp - relay_zustand_seit >= min_schaltzeit_in_min * 60;
			bool ist_winterladen = _winterladen_ist_aktiv();
			if(!schalt_mindestdauer_ist_erreicht) {
				_log(log_key, (char*) (ist_winterladen ? "-winter>SchaltdauerNichtErreicht" : "-force>SchaltdauerNichtErreicht"));
				return false;
			}
			if(!relay_ist_an) {
				if(_einschalten_wegen_lastgrenzen_verboten(verbraucher, benoetigte_leistung_in_w)) {
					lastschutz_an_func();
				} else {
					_log(log_key, (char*) (ist_winterladen ? "-winter>Start" : "-force>Start"));
					schalt_func(true);
					return true;
				}
			}
			if(
				!ist_winterladen
				&& ladestatus_seit < timestamp - cfg->ladestatus_force_dauer
			) {
				_log(log_key, (char*) "-force>Ende");
				umschalten_auf_solarladen_func();
				schalt_func(false);
				return true;
			}
			return false;
		}

		template<typename F1, typename F2>
		bool _schalte_ueberladen_automatisch(
			char* log_key,
			Local::Model::Verbraucher& verbraucher,
			int relay_zustand_seit,
			int min_schaltzeit_in_min,
			int benoetigte_leistung_in_w,
			bool relay_ist_an,
			int akku_zielladestand_in_promille,
			F1 && schalt_func,
			F2 && lastschutz_an_func
		) {
			float min_bereitgestellte_leistung = _gib_min_bereitgestellte_leistung(verbraucher, benoetigte_leistung_in_w);
			bool akku_laeuft_potentiell_in_5h_ueber = verbraucher.akku_erreicht_ladestand_in_stunden(akku_zielladestand_in_promille) <= 5;
			bool akku_unterschreitet_minimalladestand = verbraucher.akku_unterschreitet_ladestand_in_promille(
				cfg->minimaler_akku_ladestand
			);
			bool schalt_mindestdauer_ist_erreicht = timestamp - relay_zustand_seit >= min_schaltzeit_in_min * 60;
			bool unerfuellter_ladewunsch = _es_besteht_ein_unerfuellter_ladewunsch(verbraucher);
			if(!schalt_mindestdauer_ist_erreicht) {
				_log(log_key, (char*) "-ueberladen>SchaltdauerNichtErreicht");
				return false;
			}

			int sonnenuntergang_abstand_in_s = 0;
			if(verbraucher.zeitpunkt_sonnenuntergang > 0) {
				sonnenuntergang_abstand_in_s = verbraucher.zeitpunkt_sonnenuntergang - timestamp;
			}
			if(
				!relay_ist_an
				&& verbraucher.solarerzeugung_ist_aktiv()
				&& akku_laeuft_potentiell_in_5h_ueber
				&& !akku_unterschreitet_minimalladestand
				&& sonnenuntergang_abstand_in_s > 0.5 * 3600
				&& (
					(
						verbraucher.aktueller_akku_ladenstand_in_promille > 600
						&& !unerfuellter_ladewunsch
					)
					|| verbraucher.aktueller_akku_ladenstand_in_promille > 800
				)
			) {
				if(_einschalten_wegen_lastgrenzen_verboten(verbraucher, benoetigte_leistung_in_w)) {
					lastschutz_an_func();
				} else {
					_log(log_key, (char*) "-ueberladen>AnWeilGenug");
					schalt_func(true);
					return true;
				}
			}
			if(relay_ist_an) {
				if(unerfuellter_ladewunsch) {
					_log(log_key, (char*) "-ueberladen>AusWeilLadewunsch");
					schalt_func(false);
					return true;
				}
				if(
					!akku_laeuft_potentiell_in_5h_ueber
					|| akku_unterschreitet_minimalladestand
					|| !verbraucher.solarerzeugung_ist_aktiv()
				) {
					_log(log_key, (char*) "-ueberladen>AusWeilAkkuVorhersage");
					schalt_func(false);
					return true;
				}
			}
			return false;
		}

		bool _es_besteht_ein_unerfuellter_ladewunsch(Local::Model::Verbraucher& verbraucher) {
			if(
				verbraucher.auto_ladestatus == Local::Model::Verbraucher::Ladestatus::solar
				&& !verbraucher.auto_relay_ist_an
			) {
				return true;
			}
			if(
				verbraucher.roller_ladestatus == Local::Model::Verbraucher::Ladestatus::solar
				&& !verbraucher.roller_relay_ist_an
			) {
				return true;
			}
			return false;
		}

	public:
		int timestamp = 0;

		VerbraucherAPI(
			Local::Config& cfg,
			Local::Service::WebReader& web_reader,
			Local::Service::FileReader& file_reader,
			Local::Service::FileWriter& file_writer
		): BaseAPI(cfg, web_reader), file_reader(&file_reader), file_writer(file_writer) {
			_load_shelly_roller_cache();
			_load_ladeverhalten_wintermodus_cache();
			timestamp = shelly_roller_cache_timestamp;
		}

		void daten_holen_und_einsetzen(
			Local::Model::Verbraucher& verbraucher,
			Local::Model::ElektroAnlage& elektroanlage,
			Local::Model::Wetter wetter
		) {
			verbraucher.auto_lastschutz = false;
			verbraucher.roller_lastschutz = false;
			verbraucher.wasser_lastschutz = false;
			verbraucher.heizung_lastschutz = false;

			_ermittle_relay_zustaende(verbraucher);

			verbraucher.aktuelle_auto_ladeleistung_in_w = round(
				(float) (elektroanlage.l1_strom_ma + elektroanlage.l1_solarstrom_ma) / 1000 * 230
			);
			_lies_verbraucher_log(
				verbraucher.auto_ladeleistung_log_in_w,
				auto_leistung_log_filename,
				5
			);
			_schreibe_verbraucher_log(
				verbraucher.auto_ladeleistung_log_in_w,
				verbraucher.aktuelle_auto_ladeleistung_in_w,
				auto_leistung_log_filename,
				5
			);
			verbraucher.auto_benoetigte_ladeleistung_in_w = cfg->auto_benoetigte_leistung_in_w;

			verbraucher.aktuelle_roller_ladeleistung_in_w = shelly_roller_cache_power;
			_lies_verbraucher_log(
				verbraucher.roller_ladeleistung_log_in_w,
				roller_leistung_log_filename,
				5
			);
			_schreibe_verbraucher_log(
				verbraucher.roller_ladeleistung_log_in_w,
				verbraucher.aktuelle_roller_ladeleistung_in_w,
				roller_leistung_log_filename,
				5
			);
			verbraucher.roller_benoetigte_ladeleistung_in_w = roller_benoetigte_ladeleistung_in_w_cache;

			verbraucher.ladeverhalten_wintermodus = ladeverhalten_wintermodus_cache;
			verbraucher.netzbezug_in_w = elektroanlage.netzbezug_in_w;

			verbraucher.aktueller_verbrauch_in_w = elektroanlage.stromverbrauch_in_w;
			_lies_verbraucher_log(
				verbraucher.verbrauch_log_in_w,
				verbrauch_leistung_log_filename,
				5
			);
			_schreibe_verbraucher_log(
				verbraucher.verbrauch_log_in_w,
				verbraucher.aktueller_verbrauch_in_w,
				verbrauch_leistung_log_filename,
				5
			);
			verbraucher.aktuelle_erzeugung_in_w = elektroanlage.solarerzeugung_in_w;
			_lies_verbraucher_log(
				verbraucher.erzeugung_log_in_w,
				erzeugung_leistung_log_filename,
				30
			);
			_schreibe_verbraucher_log(
				verbraucher.erzeugung_log_in_w,
				verbraucher.aktuelle_erzeugung_in_w,
				erzeugung_leistung_log_filename,
				30
			);

			verbraucher.aktueller_akku_ladenstand_in_promille = elektroanlage.solarakku_ladestand_in_promille;
			verbraucher.solarerzeugung_in_w = elektroanlage.solarerzeugung_in_w;
			verbraucher.ersatzstrom_ist_aktiv = elektroanlage.ersatzstrom_ist_aktiv;
			verbraucher.zeitpunkt_sonnenuntergang = wetter.zeitpunkt_sonnenuntergang;

			verbraucher.auto_ladestatus_seit = _lese_ladestatus(verbraucher.auto_ladestatus, auto_ladestatus_filename);
			verbraucher.roller_ladestatus_seit = _lese_ladestatus(verbraucher.roller_ladestatus, roller_ladestatus_filename);
			_fuelle_akkuladestands_vorhersage(verbraucher, wetter);
		}

		void fuehre_schaltautomat_aus(Local::Model::Verbraucher& verbraucher) {
			int ausschalter_auto_relay_zustand_seit = verbraucher.auto_relay_zustand_seit;
			int ausschalter_roller_relay_zustand_seit = verbraucher.roller_relay_zustand_seit;
			int akku_zielladestand_in_promille = cfg->akku_zielladestand_in_promille;
			int auto_min_schaltzeit_in_min = cfg->auto_min_schaltzeit_in_min;
			int roller_min_schaltzeit_in_min = cfg->roller_min_schaltzeit_in_min;
			int akku_zielladestand_fuer_ueberladen_in_promille = 1000;

			if(verbraucher.ersatzstrom_ist_aktiv) {
				ladeverhalten_wintermodus_cache = 0;
				verbraucher.ladeverhalten_wintermodus = 0;
			    verbraucher.auto_ladestatus = Local::Model::Verbraucher::Ladestatus::solar;
				verbraucher.roller_ladestatus = Local::Model::Verbraucher::Ladestatus::solar;
				ausschalter_auto_relay_zustand_seit = 0;
				ausschalter_roller_relay_zustand_seit = 0;
				_log((char*) "Ersatzstrom->nurUeberschuss");
				akku_zielladestand_in_promille = 1200;
				akku_zielladestand_fuer_ueberladen_in_promille = 1200;
				auto_min_schaltzeit_in_min = 5;
				roller_min_schaltzeit_in_min = 5;
			}

			if(_ausschalten_wegen_lastgrenzen(verbraucher)) {
				if(verbraucher.auto_relay_ist_an) {
					_log((char*) "AutoLastgrenze");
					_schalte_auto_relay(false);
					verbraucher.auto_lastschutz = true;
					return;
				}
				if(verbraucher.roller_relay_ist_an) {
					_log((char*) "RollerLastgrenze");
					_schalte_roller_relay(false);
					verbraucher.roller_lastschutz = true;
					return;
				}
				if(verbraucher.wasser_relay_ist_an) {
					_log((char*) "WasserLastgrenze");
					_schalte_wasser_relay(false);
					verbraucher.wasser_lastschutz = true;
					return;
				}
				if(verbraucher.heizung_relay_ist_an) {
					_log((char*) "HeizungLastgrenze");
					_schalte_heizung_relay(false);
					verbraucher.heizung_lastschutz = true;
					return;
				}
			}

			int karenzzeit = (5 * 60);
			if(
				verbraucher.auto_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.roller_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.wasser_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.heizung_relay_zustand_seit >= timestamp - karenzzeit
			) {
				// Von Einschalten bis voller Verbrauch vergeht Zeit
				// ohne dies wird ggf mehr zugeschaltet als sinnvoll ist
				_log((char*) "SchaltKarenzzeit");
				return;
			}

			if(
				(
					verbraucher.auto_ladestatus == Local::Model::Verbraucher::Ladestatus::force
					|| _winterladen_ist_aktiv()
				) && _behandle_force_laden(
					(char*) "auto",
					verbraucher,
					verbraucher.auto_relay_zustand_seit,
					verbraucher.auto_ladestatus_seit,
					auto_min_schaltzeit_in_min,
					verbraucher.auto_benoetigte_ladeleistung_in_w,
					verbraucher.auto_relay_ist_an,
					[&](bool ein) { _schalte_auto_relay(ein); },
					[&]() { setze_auto_ladestatus(Local::Model::Verbraucher::Ladestatus::solar); },
					[&]() { verbraucher.auto_lastschutz = true; }
				)
			) {
				return;
			}
			if(
				(
					verbraucher.roller_ladestatus == Local::Model::Verbraucher::Ladestatus::force
					|| _winterladen_ist_aktiv()
				) && _behandle_force_laden(
					(char*) "roller",
					verbraucher,
					verbraucher.roller_relay_zustand_seit,
					verbraucher.roller_ladestatus_seit,
					roller_min_schaltzeit_in_min,
					verbraucher.roller_benoetigte_ladeleistung_in_w,
					verbraucher.roller_relay_ist_an,
					[&](bool ein) { _schalte_roller_relay(ein); },
					[&]() { setze_roller_ladestatus(Local::Model::Verbraucher::Ladestatus::solar); },
					[&]() { verbraucher.roller_lastschutz = true; }
				)
			) {
				return;
			}

			if(
				verbraucher.auto_ladestatus == Local::Model::Verbraucher::Ladestatus::solar
				&& !_winterladen_ist_aktiv()
				&& _behandle_solar_laden(
					(char*) "auto",
					verbraucher,
					verbraucher.auto_relay_zustand_seit,
					auto_min_schaltzeit_in_min,
					verbraucher.auto_benoetigte_ladeleistung_in_w,
					verbraucher.auto_relay_ist_an,
					akku_zielladestand_in_promille,
					[&](bool ein) { _schalte_auto_relay(ein); },
					[&]() { verbraucher.auto_lastschutz = true; }
				)
			) {
				return;
			}
			if(
				verbraucher.roller_ladestatus == Local::Model::Verbraucher::Ladestatus::solar
				&& !_winterladen_ist_aktiv()
				&& _behandle_solar_laden(
					(char*) "roller",
					verbraucher,
					verbraucher.roller_relay_zustand_seit,
					roller_min_schaltzeit_in_min,
					verbraucher.roller_benoetigte_ladeleistung_in_w,
					verbraucher.roller_relay_ist_an,
					akku_zielladestand_in_promille,
					[&](bool ein) { _schalte_roller_relay(ein); },
					[&]() { verbraucher.roller_lastschutz = true; }
				)
			) {
				return;
			}

			if(_schalte_ueberladen_automatisch(
				(char*) "wasser",
				verbraucher,
				verbraucher.wasser_relay_zustand_seit,
				cfg->wasser_min_schaltzeit_in_min,
				cfg->wasser_benoetigte_leistung_in_w,
				verbraucher.wasser_relay_ist_an,
				akku_zielladestand_fuer_ueberladen_in_promille,
				[&](bool ein) { _schalte_wasser_relay(ein); },
				[&]() { verbraucher.wasser_lastschutz = true; }
			)) {
				return;
			}
			if(_schalte_ueberladen_automatisch(
				(char*) "heizung",
				verbraucher,
				verbraucher.heizung_relay_zustand_seit,
				cfg->heizung_min_schaltzeit_in_min,
				cfg->heizung_benoetigte_leistung_in_w,
				verbraucher.heizung_relay_ist_an,
				akku_zielladestand_fuer_ueberladen_in_promille,
				[&](bool ein) { _schalte_heizung_relay(ein); },
				[&]() { verbraucher.heizung_lastschutz = true; }
			)) {
				return;
			}
		}

		void setze_roller_ladestatus(Local::Model::Verbraucher::Ladestatus status) {
			char stat[6];
			if(status == Local::Model::Verbraucher::Ladestatus::force) {
				strcpy(stat, "force");
				_log((char*) "setze_roller_ladestatus>force");
			} else {
				strcpy(stat, "solar");
				_log((char*) "setze_roller_ladestatus>solar");
			}
			if(file_writer.open_file_to_overwrite(roller_ladestatus_filename)) {
				file_writer.write_formated("%s,%i", stat, timestamp);
				file_writer.close_file();
			}
			file_writer.delete_file(roller_leistung_log_filename);
		}

		void setze_auto_ladestatus(Local::Model::Verbraucher::Ladestatus status) {
			char stat[6];
			if(status == Local::Model::Verbraucher::Ladestatus::force) {
				strcpy(stat, "force");
				_log((char*) "setze_auto_ladestatus>force");
			} else {
				strcpy(stat, "solar");
				_log((char*) "setze_auto_ladestatus>solar");
			}
			if(file_writer.open_file_to_overwrite(auto_ladestatus_filename)) {
				file_writer.write_formated("%s,%i", stat, timestamp);
				file_writer.close_file();
			}
			file_writer.delete_file(auto_leistung_log_filename);
		}

		void wechsle_roller_ladeleistung() {
			_log((char*) "wechsle_roller_ladeleistung");
			int roller_benoetigte_ladeleistung_in_w = roller_benoetigte_ladeleistung_in_w_cache;
			if(roller_benoetigte_ladeleistung_in_w == cfg->roller_benoetigte_leistung_hoch_in_w) {
				roller_benoetigte_ladeleistung_in_w = cfg->roller_benoetigte_leistung_gering_in_w;
			} else {
				roller_benoetigte_ladeleistung_in_w = cfg->roller_benoetigte_leistung_hoch_in_w;
			}
			if(file_writer.open_file_to_overwrite(roller_leistung_filename)) {
				file_writer.write_formated("%d", roller_benoetigte_ladeleistung_in_w);
				file_writer.close_file();
			}
		}

		void wechsle_auto_ladeverhalten() {
			_log((char*) "wechsle_ladeverhalten_wintermodus");
			int ladeverhalten_wintermodus = ladeverhalten_wintermodus_cache;
			if(ladeverhalten_wintermodus) {
				ladeverhalten_wintermodus = 0;
			} else {
				ladeverhalten_wintermodus = 1;
			}
			if(file_writer.open_file_to_overwrite(ladeverhalten_wintermodus_filename)) {
				file_writer.write_formated("%d", ladeverhalten_wintermodus);
				file_writer.close_file();
			}
		}

		void starte_router_neu() {
			_log((char*) "starte_router_neu");
			_schalte_shellyplug(
				false, cfg->router_neustart_relay_host, cfg->router_neustart_relay_port,
				web_reader->default_timeout_in_hundertstel_s
			);
			// Schaltet allein wieder ein (killt ja das netz, was ein Einschalten unmöglich macht)
		}
	};
}
