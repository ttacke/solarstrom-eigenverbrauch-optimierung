#pragma once
#include "base_api.h"

namespace Local {
	class VerbraucherAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		Local::Service::FileReader* file_reader;
		Local::Service::FileWriter& file_writer;
		int shelly_roller_cache_timestamp = 0;
		bool shelly_roller_cache_ison = false;
		int shelly_roller_cache_power = 0;
		int roller_benoetigte_ladeleistung_in_w_cache = 0;

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

		const char* verbrauch_leistung_log_filename = "verbrauch_leistung.log";
		const char* erzeugung_leistung_log_filename = "erzeugung_leistung.log";

		const char* log_filename = "verbraucher_automatisierung.log";

		void _log(char* msg) {
			if(file_writer.open_file_to_append(log_filename)) {
				file_writer.write_formated("%d:%s\n", timestamp, msg);
				file_writer.close_file();
			}
		}

		void _log(char* key, char* msg) {
			if(file_writer.open_file_to_append(log_filename)) {
				file_writer.write_formated("%d:%s>%s\n", timestamp, key, msg);
				file_writer.close_file();
			}
		}

		bool _laden_ist_beendet(
			char* log_key,
			int relay_zustand_seit,
			int min_schaltzeit_in_min,
			bool relay_ist_an,
			bool es_laedt
		) {
			bool schalt_mindestdauer_ist_erreicht = timestamp - relay_zustand_seit >= min_schaltzeit_in_min * 60;
			if(!schalt_mindestdauer_ist_erreicht) {
				_log(log_key, (char*) "_laden_ist_beendet>schaltdauerNichtErreicht");
				return false;
			}
			if(!relay_ist_an) {
				_log(log_key, (char*) "_laden_ist_beendet>StatusFehlerKorrigiert");
				return true;
			}
			if(!es_laedt) {
				_log(log_key, (char*) "_laden_ist_beendet>AusWeilBeendet");
				return true;
			}
			return false;
		}

		float _gib_min_bereitgestellte_leistung(Local::Verbraucher& verbraucher, int benoetigte_leistung_in_w) {
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

		template<typename F1>
		bool _schalte_automatisch(
			char* log_key,
			Local::Verbraucher& verbraucher,
			int relay_zustand_seit,
			int min_schaltzeit_in_min,
			int benoetigte_leistung_in_w,
			bool relay_ist_an,
			int akku_zielladestand_in_promille,
			F1 && schalt_func
		) {
			float min_bereitgestellte_leistung = _gib_min_bereitgestellte_leistung(
				verbraucher, benoetigte_leistung_in_w
			);
			bool akku_erreicht_zielladestand = verbraucher.akku_erreicht_ladestand_in_promille(
				akku_zielladestand_in_promille
			);
			bool schalt_mindestdauer_ist_erreicht = timestamp - relay_zustand_seit >= min_schaltzeit_in_min * 60;
			float nicht_laden_unter_akkuladestand = 400;
			float einschaltschwelle = _ermittle_solarladen_einschaltschwelle(
				verbraucher.aktueller_akku_ladenstand_in_promille,
				nicht_laden_unter_akkuladestand
			);
			if(!schalt_mindestdauer_ist_erreicht) {
				_log(log_key, (char*) "-solar>SchaltdauerNichtErreicht");
				return false;
			}

			int sonnenuntergang_abstand_in_s = 0;
			if(verbraucher.zeitpunkt_sonnenuntergang > 0) {
				sonnenuntergang_abstand_in_s = verbraucher.zeitpunkt_sonnenuntergang - timestamp;
			}

			if(
				!relay_ist_an
				&& verbraucher.solarerzeugung_ist_aktiv()
				&& sonnenuntergang_abstand_in_s > 0.5 * 3600
				&& akku_erreicht_zielladestand
				&& min_bereitgestellte_leistung > einschaltschwelle
			) {
				_log(log_key, (char*) "-solar>AnWeilGenug");
				schalt_func(true);
				return true;
			}
			if(relay_ist_an) {
				if(!akku_erreicht_zielladestand || !verbraucher.solarerzeugung_ist_aktiv()) {
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

			verbraucher.roller_relay_ist_an = shelly_roller_cache_ison;
			verbraucher.roller_relay_zustand_seit = _lese_zustand_seit(roller_relay_zustand_seit_filename);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
		}

		bool _netz_relay_ist_an(const char* host, int port) {
			web_reader->send_http_get_request(
				host,
				port,
				"/relay"
			);
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

		int _gib_auto_benoetigte_ladeleistung_in_w() {
			int leistung = cfg->auto_benoetigte_leistung_gering_in_w;
			if(file_reader->open_file_to_read(auto_leistung_filename)) {
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

		void _schalte_roller_relay(bool ein) {
			_schalte_shellyplug(
				ein, cfg->roller_relay_host, cfg->roller_relay_port,
				web_reader->default_timeout_in_hundertstel_s
			);
			if(roller_benoetigte_ladeleistung_in_w_cache == cfg->auto_benoetigte_leistung_gering_in_w) {
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

		void _lese_ladestatus(Local::Verbraucher::Ladestatus& ladestatus, const char* filename) {
			ladestatus = Local::Verbraucher::Ladestatus::solar;
			if(file_reader->open_file_to_read(filename)) {
				while(file_reader->read_next_block_to_buffer()) {
					if(file_reader->find_in_buffer((char*) "([a-z]+)")) {
						if(strcmp(file_reader->finding_buffer, "force") == 0) {
							ladestatus = Local::Verbraucher::Ladestatus::force;
						}
						return;
					}
				}
				file_reader->close_file();
			}
		}

		void _fuelle_akkuladestands_vorhersage(
			Local::Verbraucher& verbraucher,
			Local::Wetter wetter
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
			bool senden_erfolgreich = false;
			if(roller_benoetigte_ladeleistung_in_w_cache == cfg->auto_benoetigte_leistung_gering_in_w) {
				senden_erfolgreich = web_reader->send_http_get_request(
					cfg->roller_aussen_relay_host,
					cfg->roller_aussen_relay_port,
					"/status"
				);
			}
			if(!senden_erfolgreich) {// Internen immer abfragen, damit min. ein Datensatz da ist
				web_reader->send_http_get_request(
					cfg->roller_relay_host,
					cfg->roller_relay_port,
					"/status"
				);
			}

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
			timestamp = shelly_roller_cache_timestamp;
		}

		void daten_holen_und_einsetzen(
			Local::Verbraucher& verbraucher,
			Local::ElektroAnlage& elektroanlage,
			Local::Wetter wetter
		) {
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
			verbraucher.auto_benoetigte_ladeleistung_in_w = _gib_auto_benoetigte_ladeleistung_in_w();

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

			_lese_ladestatus(verbraucher.auto_ladestatus, auto_ladestatus_filename);
			_lese_ladestatus(verbraucher.roller_ladestatus, roller_ladestatus_filename);
			_fuelle_akkuladestands_vorhersage(verbraucher, wetter);
		}

		void fuehre_schaltautomat_aus(Local::Verbraucher& verbraucher) {
			int ausschalter_auto_relay_zustand_seit = verbraucher.auto_relay_zustand_seit;
			int ausschalter_roller_relay_zustand_seit = verbraucher.roller_relay_zustand_seit;
			int akku_zielladestand_in_promille = cfg->akku_zielladestand_in_promille;
			int auto_min_schaltzeit_in_min = cfg->auto_min_schaltzeit_in_min;
			int roller_min_schaltzeit_in_min = cfg->roller_min_schaltzeit_in_min;
			int akku_zielladestand_fuer_ueberladen_in_promille = 1000;
			if(verbraucher.ersatzstrom_ist_aktiv) {
				verbraucher.auto_ladestatus = Local::Verbraucher::Ladestatus::solar;
				verbraucher.roller_ladestatus = Local::Verbraucher::Ladestatus::solar;
				ausschalter_auto_relay_zustand_seit = 0;
				ausschalter_roller_relay_zustand_seit = 0;
				_log((char*) "Ersatzstrom->forceUnterbinden");
				akku_zielladestand_in_promille = 1200;
				akku_zielladestand_fuer_ueberladen_in_promille = 1200;
				auto_min_schaltzeit_in_min = 5;
				roller_min_schaltzeit_in_min = 5;
			}
			if(
				verbraucher.auto_ladestatus == Local::Verbraucher::Ladestatus::force
				&& _laden_ist_beendet(
					(char*) "auto",
					ausschalter_auto_relay_zustand_seit,
					cfg->auto_min_schaltzeit_in_min,
					verbraucher.auto_relay_ist_an,
					verbraucher.auto_laden_ist_an()
				)
			) {
				setze_auto_ladestatus(Local::Verbraucher::Ladestatus::solar);
				return;
			}

			if(
				verbraucher.roller_ladestatus == Local::Verbraucher::Ladestatus::force
				&& _laden_ist_beendet(
					(char*) "roller",
					ausschalter_roller_relay_zustand_seit,
					cfg->roller_min_schaltzeit_in_min,
					verbraucher.roller_relay_ist_an,
					verbraucher.roller_laden_ist_an()
				)
			) {
				setze_roller_ladestatus(Local::Verbraucher::Ladestatus::solar);
				return;
			}

			int karenzzeit = (7 * 60);// in Sekunden, muss >5min sein, da die LadeLog 5min betraegt
			if(
				verbraucher.auto_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.roller_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.wasser_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.heizung_relay_zustand_seit >= timestamp - karenzzeit
			) {
				_log((char*) "SchaltKarenzzeit");
				return;
			}

			if(
				verbraucher.auto_ladestatus == Local::Verbraucher::Ladestatus::solar
				&& _schalte_automatisch(
					(char*) "auto",
					verbraucher,
					verbraucher.auto_relay_zustand_seit,
					auto_min_schaltzeit_in_min,
					verbraucher.auto_benoetigte_ladeleistung_in_w,
					verbraucher.auto_relay_ist_an,
					akku_zielladestand_in_promille,
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
					roller_min_schaltzeit_in_min,
					verbraucher.roller_benoetigte_ladeleistung_in_w,
					verbraucher.roller_relay_ist_an,
					akku_zielladestand_in_promille,
					[&](bool ein) { _schalte_roller_relay(ein); }
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
				[&](bool ein) { _schalte_wasser_relay(ein); }
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
				[&](bool ein) { _schalte_heizung_relay(ein); }
			)) {
				return;
			}
		}

		template<typename F1>
		bool _schalte_ueberladen_automatisch(
			char* log_key,
			Local::Verbraucher& verbraucher,
			int relay_zustand_seit,
			int min_schaltzeit_in_min,
			int benoetigte_leistung_in_w,
			bool relay_ist_an,
			int akku_zielladestand_in_promille,
			F1 && schalt_func
		) {
			float min_bereitgestellte_leistung = _gib_min_bereitgestellte_leistung(verbraucher, benoetigte_leistung_in_w);
			bool akku_laeuft_potentiell_in_3h_ueber = verbraucher.akku_erreicht_ladestand_in_stunden(akku_zielladestand_in_promille) <= 3;
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
				&& !unerfuellter_ladewunsch
				&& verbraucher.solarerzeugung_ist_aktiv()
				&& sonnenuntergang_abstand_in_s > 0.5 * 3600
				&& (
					(
						akku_laeuft_potentiell_in_3h_ueber
						&& verbraucher.aktueller_akku_ladenstand_in_promille > 400
					) || (
						verbraucher.aktueller_akku_ladenstand_in_promille > 950
						&& min_bereitgestellte_leistung > 0.5
					)
				)
			) {
				_log(log_key, (char*) "-ueberladen>AnWeilGenug");
				schalt_func(true);
				return true;
			}
			if(relay_ist_an) {
				if(unerfuellter_ladewunsch) {
					_log(log_key, (char*) "-ueberladen>AusWeilLadewunsch");
					schalt_func(false);
					return true;
				}
				if(!akku_laeuft_potentiell_in_3h_ueber || !verbraucher.solarerzeugung_ist_aktiv()) {
					_log(log_key, (char*) "-ueberladen>AusWeilAkkuVorhersage");
					schalt_func(false);
					return true;
				}
			}
			return false;
		}

		bool _es_besteht_ein_unerfuellter_ladewunsch(Local::Verbraucher& verbraucher) {
			if(
				verbraucher.auto_ladestatus == Local::Verbraucher::Ladestatus::solar
				&& !verbraucher.auto_relay_ist_an
			) {
				return true;
			}
			if(
				verbraucher.roller_ladestatus == Local::Verbraucher::Ladestatus::solar
				&& !verbraucher.roller_relay_ist_an
			) {
				return true;
			}
			return false;
		}

		void setze_roller_ladestatus(Local::Verbraucher::Ladestatus status) {
			char stat[6];
			if(status == Local::Verbraucher::Ladestatus::force) {
				_schalte_roller_relay(true);
				strcpy(stat, "force");
				_log((char*) "setze_roller_ladestatus>force");
			} else {
				strcpy(stat, "solar");
				_log((char*) "setze_roller_ladestatus>solar");
			}
			if(file_writer.open_file_to_overwrite(roller_ladestatus_filename)) {
				file_writer.write_formated("%s", stat);
				file_writer.close_file();
			}
			file_writer.delete_file(roller_leistung_log_filename);
		}

		void setze_auto_ladestatus(Local::Verbraucher::Ladestatus status) {
			char stat[6];
			if(status == Local::Verbraucher::Ladestatus::force) {
				_schalte_auto_relay(true);
				strcpy(stat, "force");
				_log((char*) "setze_auto_ladestatus>force");
			} else {
				strcpy(stat, "solar");
				_log((char*) "setze_auto_ladestatus>solar");
			}
			if(file_writer.open_file_to_overwrite(auto_ladestatus_filename)) {
				file_writer.write_formated("%s", stat);
				file_writer.close_file();
			}
			file_writer.delete_file(auto_leistung_log_filename);
		}

		void wechsle_auto_ladeleistung() {
			_log((char*) "wechsle_auto_ladeleistung");
			int auto_benoetigte_ladeleistung_in_w = _gib_auto_benoetigte_ladeleistung_in_w();
			if(auto_benoetigte_ladeleistung_in_w == cfg->auto_benoetigte_leistung_hoch_in_w) {
				auto_benoetigte_ladeleistung_in_w = cfg->auto_benoetigte_leistung_gering_in_w;
			} else {
				auto_benoetigte_ladeleistung_in_w = cfg->auto_benoetigte_leistung_hoch_in_w;
			}
			if(file_writer.open_file_to_overwrite(auto_leistung_filename)) {
				file_writer.write_formated("%d", auto_benoetigte_ladeleistung_in_w);
				file_writer.close_file();
			}
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
	};
}
