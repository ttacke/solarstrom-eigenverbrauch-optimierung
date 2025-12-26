#pragma once
#include "base_api.h"
#include "../model/shelly.h"

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
		bool ladeverhalten_wintermodus = false;
		int heizstab_relay_maximale_wartezeit = 3600;

		const char* roller_ladestatus_filename = "roller.ladestatus";
		const char* roller_leistung_filename = "roller_leistung.status";
		const char* auto_ladestatus_filename = "auto.ladestatus";
		const char* automatisierung_log_filename_template = "verbraucher_automatisierung-%4d-%02d.log";

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
			int aktueller_akku_ladenstand_in_promille
		) {
			if(aktueller_akku_ladenstand_in_promille < cfg->nicht_laden_unter_akkuladestand_in_promille) {
				return 99999;
			}
			float einschaltschwelle =
				1.1 - (0.016 * (
					(aktueller_akku_ladenstand_in_promille - cfg->nicht_laden_unter_akkuladestand_in_promille)
					* 100 /
					(cfg->akku_zielladestand_in_promille - cfg->nicht_laden_unter_akkuladestand_in_promille)
				));
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
			int *frueh_leeren_zuletzt_gestartet_an_timestamp,
			bool *frueh_leeren_ist_aktiv,
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
				cfg->minimal_im_tagesgang_erreichbarer_akku_ladestand_in_promille
			);
			float einschaltschwelle = _ermittle_solarladen_einschaltschwelle(
				verbraucher.aktueller_akku_ladenstand_in_promille
			);

			int sonnenuntergang_abstand_in_s = 0;
			if(verbraucher.zeitpunkt_sonnenuntergang > 0) {
				sonnenuntergang_abstand_in_s = verbraucher.zeitpunkt_sonnenuntergang - timestamp;
			}

			bool akku_ist_zu_voll = verbraucher.aktueller_akku_ladenstand_in_promille >= cfg->akku_ladestand_in_promille_fuer_erzwungenes_laden ? true : false;

			if(
				day(*frueh_leeren_zuletzt_gestartet_an_timestamp) != day(timestamp)
				&& !*frueh_leeren_ist_aktiv
				&& !relay_ist_an
				&& hour(timestamp) == cfg->frueh_leeren_starte_in_stunde_utc
				&& akku_erreicht_zielladestand
				&& !akku_unterschreitet_minimalladestand
				&&
					verbraucher.aktueller_akku_ladenstand_in_promille
					>=
					cfg->minimal_im_tagesgang_erreichbarer_akku_ladestand_in_promille + start_puffer_in_promille
			) {
				if(_einschalten_wegen_lastgrenzen_verboten(verbraucher, benoetigte_leistung_in_w)) {
					lastschutz_an_func();
				} else {
					_log(log_key, (char*) "-solar>FruehLeerenAn");
					*frueh_leeren_zuletzt_gestartet_an_timestamp = timestamp;
					*frueh_leeren_ist_aktiv = true;
					schalt_func(true);
					return true;
				}
			}

			if(*frueh_leeren_ist_aktiv) {
				if(
					relay_ist_an
					&& (
						!akku_erreicht_zielladestand
						|| akku_unterschreitet_minimalladestand
						||
							verbraucher.aktueller_akku_ladenstand_in_promille
							<
							cfg->minimal_im_tagesgang_erreichbarer_akku_ladestand_in_promille
					)
				) {
					_log(log_key, (char*) "-solar>FruehLeerenAus");
					*frueh_leeren_zuletzt_gestartet_an_timestamp = timestamp;
					*frueh_leeren_ist_aktiv = false;
					schalt_func(false);
					return true;
				}
				return false;// Weitere Steuerung in der Zeit unterbinden
			}

			if(
				!relay_ist_an
				&& verbraucher.solarerzeugung_ist_aktiv()
				&& sonnenuntergang_abstand_in_s > 0.5 * 3600
				&& (
					(akku_erreicht_zielladestand && !akku_unterschreitet_minimalladestand)
					|| akku_ist_zu_voll
				)
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
					(
						(!akku_erreicht_zielladestand || akku_unterschreitet_minimalladestand)
						&& !akku_ist_zu_voll
					)
					|| !verbraucher.solarerzeugung_ist_aktiv()
				) {
					_log(log_key, (char*) "-aus>AusWegenZielladestand");
					schalt_func(false);
					return true;
				}
				if(verbraucher.aktueller_akku_ladenstand_in_promille < cfg->nicht_laden_unter_akkuladestand_in_promille) {
					_log(log_key, (char*) "-aus>AusWegenZuWenigAkku");
					schalt_func(false);
					return true;
				}
			}
			return false;
		}

		void _ermittle_relay_zustaende(Local::Model::Verbraucher& verbraucher) {
			verbraucher.heizung_relay_ist_an = _netz_relay_ist_an(cfg->heizung_relay_host, cfg->heizung_relay_port);
			verbraucher.heizung_relay_zustand_seit = Local::SemipersistentData::heizung_relay_zustand_seit;
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher.wasser_relay_ist_an = _netz_relay_ist_an(cfg->wasser_relay_host, cfg->wasser_relay_port);
			verbraucher.wasser_relay_zustand_seit = Local::SemipersistentData::wasser_relay_zustand_seit;
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			// Wird invertiert, damit der Zustand ohne Steuerung der normale Wallboxbetrieb ist
			verbraucher.auto_relay_ist_an = !_netz_relay_ist_an(cfg->auto_relay_host, cfg->auto_relay_port);
			verbraucher.auto_relay_zustand_seit = Local::SemipersistentData::auto_relay_zustand_seit;
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher.roller_relay_ist_an = shelly_roller_cache_ison;
			verbraucher.roller_relay_zustand_seit = Local::SemipersistentData::roller_relay_zustand_seit;
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			Local::Model::Shelly shelly_daten;
			if(_read_shelly_content(
				(char*) cfg->heizung_luftvorwaermer_relay_host,
				cfg->heizung_luftvorwaermer_relay_port,
				shelly_daten
			)) {
				verbraucher.heizung_luftvorwaermer_relay_ist_an = shelly_daten.ison;
				verbraucher.heizung_luftvorwaermer_aktuelle_leistung_in_w = shelly_daten.power;
			}
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
			if(_read_shelly_content(
				(char*) cfg->wasser_begleitheizung_relay_host,
				cfg->wasser_begleitheizung_relay_port,
				shelly_daten
			)) {
				verbraucher.wasser_begleitheizung_relay_is_an = shelly_daten.ison;
				verbraucher.wasser_begleitheizung_aktuelle_leistung_in_w = shelly_daten.power;
			}
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
			Local::SemipersistentData::roller_relay_zustand_seit = timestamp;
		}

		void _schalte_auto_relay(bool ein) {
			// Wird invertiert, damit der Zustand ohne Steuerung der normale Wallboxbetrieb ist
			_schalte_netz_relay(ein ? false : true, cfg->auto_relay_host, cfg->auto_relay_port);
			Local::SemipersistentData::auto_relay_zustand_seit = timestamp;
		}

		void _schalte_wasser_relay(bool ein) {
			_schalte_netz_relay(ein, cfg->wasser_relay_host, cfg->wasser_relay_port);
			Local::SemipersistentData::wasser_relay_zustand_seit = timestamp;
		}

		void _schalte_heizung_relay(bool ein) {
			_schalte_netz_relay(ein, cfg->heizung_relay_host, cfg->heizung_relay_port);
			Local::SemipersistentData::heizung_relay_zustand_seit = timestamp;
		}

		void _schalte_heizstab_relay(bool ein) {
			_log((char*) "schalte heizstab: ", (char*) (ein ? "erlaubt" : "aus"));
			_schalte_shellyplug(
				ein, cfg->heizstab_relay_host, cfg->heizstab_relay_port,
				web_reader->default_timeout_in_hundertstel_s
			);
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

		void _schalte_roller_ladeort_und_leistung_auf_aussen(bool aussen_und_gering) {
			if(
				(
					aussen_und_gering
					&& roller_benoetigte_ladeleistung_in_w_cache == cfg->roller_benoetigte_leistung_gering_in_w
				) || (
					!aussen_und_gering
					&& roller_benoetigte_ladeleistung_in_w_cache == cfg->roller_benoetigte_leistung_hoch_in_w
				)
			) {
				return;
			}

			_log((char*) "schalte_roller_ladeort_und_leistung:", (char*) (aussen_und_gering ? "aussen+gering" : "innen+hoch"));
			if(aussen_und_gering) {
				roller_benoetigte_ladeleistung_in_w_cache = cfg->roller_benoetigte_leistung_gering_in_w;
			} else {
				roller_benoetigte_ladeleistung_in_w_cache = cfg->roller_benoetigte_leistung_hoch_in_w;
			}
			if(file_writer.open_file_to_overwrite(roller_leistung_filename)) {
				file_writer.write_formated("%d", roller_benoetigte_ladeleistung_in_w_cache);
				file_writer.close_file();
			}
		}

		void _schreibe_verbraucher_log(
			int* liste, int aktuell, int length
		) {
			for(int i = 1; i < length; i++) {
				liste[i - 1] = liste[i];
			}
			liste[length - 1] = aktuell;
		}

		int _lese_ladestatus(Local::Model::Verbraucher::Ladestatus& ladestatus, const char* filename) {
			if(file_reader->open_file_to_read(filename)) {
				while(file_reader->read_next_block_to_buffer()) {
					if(file_reader->find_in_buffer((char*) "([a-z]+),")) {
						if(strcmp(file_reader->finding_buffer, "force") == 0) {
							ladestatus = Local::Model::Verbraucher::Ladestatus::force;
							if(file_reader->find_in_buffer((char*) ",([0-9]+)")) {
								return atoi(file_reader->finding_buffer);
							}
						}
						if(ladestatus != Local::Model::Verbraucher::Ladestatus::solar) {
							_log((char*) "LadestatusWechsel>solar", (char*) filename);
							ladestatus = Local::Model::Verbraucher::Ladestatus::solar;
						}
						return 0;
					}
				}
				file_reader->close_file();
			} else {
				_log((char*) "Lesefehler", (char*) filename);
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
			float solarstrahlungs_vorhersage_umrechnungsfaktor = cfg->solarstrahlungs_vorhersage_umrechnungsfaktor_sommer;
			int grundverbrauch_in_w_pro_h = cfg->grundverbrauch_in_w_pro_h_sommer;
			if(month(timestamp) <= 2 || month(timestamp) >= 10) {
				solarstrahlungs_vorhersage_umrechnungsfaktor = cfg->solarstrahlungs_vorhersage_umrechnungsfaktor_winter;
				grundverbrauch_in_w_pro_h = cfg->grundverbrauch_in_w_pro_h_winter;
			}
			for(int i = 0; i < 12; i++) {
				int akku_veraenderung_in_promille = round(
					(
						(float) wetter.stundenvorhersage_solarstrahlung_liste[i]
						*
						solarstrahlungs_vorhersage_umrechnungsfaktor
						- grundverbrauch_in_w_pro_h
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

		void _load_shelly_roller_cache(int beat_count) {
			roller_benoetigte_ladeleistung_in_w_cache = _gib_roller_benoetigte_ladeleistung_in_w();
			bool erfolgreich = false;
			bool aussen_und_gering = false;
			if(roller_benoetigte_ladeleistung_in_w_cache == cfg->roller_benoetigte_leistung_gering_in_w) {
				aussen_und_gering = true;
			}
			if(!aussen_und_gering && (beat_count % 60 == 0 || beat_count == 3)) {
				Serial.println("Teste Roller-Aussen-Shelly");
				aussen_und_gering = true;
			}

			Local::Model::Shelly shelly_daten;
			if(aussen_und_gering) {
				if(_read_shelly_content(
					(char*) cfg->roller_aussen_relay_host,
					cfg->roller_aussen_relay_port,
					shelly_daten
				)) {
					erfolgreich = true;
					_schalte_roller_ladeort_und_leistung_auf_aussen(true);
				}
			}
			if(!erfolgreich) {// Innen immer abfragen, damit min. ein Datensatz da ist
				_read_shelly_content(
					(char*) cfg->roller_relay_host,
					cfg->roller_relay_port,
					shelly_daten
				);
				_schalte_roller_ladeort_und_leistung_auf_aussen(false);
			}
			shelly_roller_cache_timestamp = shelly_daten.timestamp;
			shelly_roller_cache_ison = shelly_daten.ison;
			shelly_roller_cache_power = shelly_daten.power;
		}

		bool _read_shelly_content(char* host, int port, Local::Model::Shelly& shelly) {
			if(!web_reader->send_http_get_request(host, port, "/status")) {
				return false;
			}
			shelly.timestamp = 0;
			while(web_reader->read_next_block_to_buffer()) {
				if(web_reader->find_in_buffer((char*) "\"unixtime\":([0-9]+)[^0-9]")) {
					shelly.timestamp = atoi(web_reader->finding_buffer);
				}
				if(web_reader->find_in_buffer((char*) "\"ison\":true")) {
					shelly.ison = true;
				}
				if(web_reader->find_in_buffer((char*) "\"power\":([0-9]+)[^0-9]")) {
					shelly.power = atoi(web_reader->finding_buffer);
				}
			}
			if(shelly.timestamp != 0) {
				return true;
			}
			return false;
		}

		bool _winterladen_ist_aktiv() {
			if(ladeverhalten_wintermodus) {
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
				cfg->minimal_im_tagesgang_erreichbarer_akku_ladestand_in_promille
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
			bool akku_ist_zu_voll = verbraucher.aktueller_akku_ladenstand_in_promille >= cfg->akku_ladestand_in_promille_fuer_erzwungenes_ueberladen ? true : false;
			if(
				!relay_ist_an
				&& verbraucher.solarerzeugung_ist_aktiv()
				&& (
					(akku_laeuft_potentiell_in_5h_ueber && !akku_unterschreitet_minimalladestand)
					|| akku_ist_zu_voll
				)
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
					(
						(!akku_laeuft_potentiell_in_5h_ueber || akku_unterschreitet_minimalladestand)
						&& !akku_ist_zu_voll
					)
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
			Local::Service::FileWriter& file_writer,
			int beat_count
		): BaseAPI(cfg, web_reader), file_reader(&file_reader), file_writer(file_writer) {
			_load_shelly_roller_cache(beat_count);
			timestamp = shelly_roller_cache_timestamp;
			ladeverhalten_wintermodus = false;
			if(
				month(timestamp) <= 2
				|| month(timestamp) == 3 && day(timestamp) < 15
				|| month(timestamp) >= 10
			) {
				ladeverhalten_wintermodus = true;
			}
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

			int auto_ladeleistung_temp = elektroanlage.l1_strom_ma + elektroanlage.l1_solarstrom_ma;
			if(cfg->auto_benutzter_leiter == 2) {
				auto_ladeleistung_temp = elektroanlage.l2_strom_ma + elektroanlage.l2_solarstrom_ma;
			} else if(cfg->auto_benutzter_leiter == 3) {
				auto_ladeleistung_temp = elektroanlage.l3_strom_ma + elektroanlage.l3_solarstrom_ma;
			}
			verbraucher.aktuelle_auto_ladeleistung_in_w = round(
				(float) auto_ladeleistung_temp / 1000 * 230
			);
			_schreibe_verbraucher_log(
				Local::SemipersistentData::auto_ladeleistung_log_in_w,
				verbraucher.aktuelle_auto_ladeleistung_in_w,
				5
			);
			verbraucher.auto_benoetigte_ladeleistung_in_w = cfg->auto_benoetigte_leistung_in_w;

			verbraucher.aktuelle_roller_ladeleistung_in_w = shelly_roller_cache_power;
			_schreibe_verbraucher_log(
				Local::SemipersistentData::roller_ladeleistung_log_in_w,
				verbraucher.aktuelle_roller_ladeleistung_in_w,
				5
			);
			verbraucher.roller_benoetigte_ladeleistung_in_w = roller_benoetigte_ladeleistung_in_w_cache;

			verbraucher.ladeverhalten_wintermodus = ladeverhalten_wintermodus;
			verbraucher.netzbezug_in_w = elektroanlage.netzbezug_in_w;

			verbraucher.aktueller_verbrauch_in_w = elektroanlage.stromverbrauch_in_w;
			_schreibe_verbraucher_log(
				Local::SemipersistentData::verbrauch_log_in_w,
				verbraucher.aktueller_verbrauch_in_w,
				5
			);
			verbraucher.aktuelle_erzeugung_in_w = elektroanlage.solarerzeugung_in_w;
			_schreibe_verbraucher_log(
				Local::SemipersistentData::erzeugung_log_in_w,
				verbraucher.aktuelle_erzeugung_in_w,
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
			int akku_zielladestand_fuer_ueberladen_in_promille = cfg->akku_zielladestand_fuer_ueberladen_in_promille;

			if(verbraucher.ersatzstrom_ist_aktiv) {
				ladeverhalten_wintermodus = false;
				verbraucher.ladeverhalten_wintermodus = 0;
			    verbraucher.auto_ladestatus = Local::Model::Verbraucher::Ladestatus::solar;
				verbraucher.roller_ladestatus = Local::Model::Verbraucher::Ladestatus::solar;
				ausschalter_auto_relay_zustand_seit = 0;
				ausschalter_roller_relay_zustand_seit = 0;
				_log((char*) "Ersatzstrom->nurUeberschuss");
				akku_zielladestand_in_promille = round((float) akku_zielladestand_in_promille * 1.4);
				akku_zielladestand_fuer_ueberladen_in_promille = round((float) akku_zielladestand_fuer_ueberladen_in_promille * 1.2);
				auto_min_schaltzeit_in_min = 5;
				roller_min_schaltzeit_in_min = 5;
			}

			if(_ausschalten_wegen_lastgrenzen(verbraucher)) {
				if(verbraucher.auto_relay_ist_an) {
					_log((char*) "AutoLastgrenze");
					_schalte_auto_relay(false);
					verbraucher.auto_lastschutz = true;
				}
				if(verbraucher.roller_relay_ist_an) {
					_log((char*) "RollerLastgrenze");
					_schalte_roller_relay(false);
					verbraucher.roller_lastschutz = true;
				}
				if(verbraucher.wasser_relay_ist_an) {
					_log((char*) "WasserLastgrenze");
					_schalte_wasser_relay(false);
					verbraucher.wasser_lastschutz = true;
				}
				if(verbraucher.heizung_relay_ist_an) {
					_log((char*) "HeizungLastgrenze");
					_schalte_heizung_relay(false);
					verbraucher.heizung_lastschutz = true;
				}
				if(verbraucher.heizstabbetrieb_ist_erlaubt) {
					_log((char*) "HeizstabLastgrenze");
					verbraucher.heizstabbetrieb_ist_erlaubt = false;
					_schalte_heizstab_relay(verbraucher.heizstabbetrieb_ist_erlaubt);
				}
				return;
			}

			int karenzzeit = (5 * 60);
			if(
				verbraucher.auto_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.roller_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.wasser_relay_zustand_seit >= timestamp - karenzzeit
				|| verbraucher.heizung_relay_zustand_seit >= timestamp - karenzzeit
				|| Local::SemipersistentData::heizstab_relay_zustand_seit >= timestamp - karenzzeit
			) {
				// Von Einschalten bis voller Verbrauch vergeht Zeit
				// ohne dies wird ggf mehr zugeschaltet als sinnvoll ist
				_log((char*) "SchaltKarenzzeit");
				return;
			}

			if(
				!_einschalten_wegen_lastgrenzen_verboten(
					verbraucher, cfg->heizstab_benoetigte_leistung_in_w
				)
				&& (
					verbraucher.heizungs_temperatur_differenz <= cfg->heizstab_einschalt_differenzwert
					||
					verbraucher.aktueller_akku_ladenstand_in_promille >= akku_zielladestand_fuer_ueberladen_in_promille
				)
			) {
				verbraucher.heizstabbetrieb_ist_erlaubt = true;
			} else if(
				verbraucher.heizungs_temperatur_differenz >= cfg->heizstab_ausschalt_differenzwert
			) {
				verbraucher.heizstabbetrieb_ist_erlaubt = false;
			}
			if(
				timestamp - Local::SemipersistentData::heizstab_relay_zustand_seit > heizstab_relay_maximale_wartezeit
				|| verbraucher.heizstabbetrieb_ist_erlaubt != Local::SemipersistentData::heizstabbetrieb_letzter_zustand
			) {
				Local::SemipersistentData::heizstab_relay_zustand_seit = timestamp;
				_schalte_heizstab_relay(verbraucher.heizstabbetrieb_ist_erlaubt);
				Local::SemipersistentData::heizstabbetrieb_letzter_zustand = verbraucher.heizstabbetrieb_ist_erlaubt;
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
					&Local::SemipersistentData::frueh_leeren_auto_zuletzt_gestartet_an_timestamp,
					&Local::SemipersistentData::frueh_leeren_auto_ist_aktiv,
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
					&Local::SemipersistentData::frueh_leeren_roller_zuletzt_gestartet_an_timestamp,
					&Local::SemipersistentData::frueh_leeren_roller_ist_aktiv,
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
			for(int i = 0; i < 5; i++) {
				Local::SemipersistentData::roller_ladeleistung_log_in_w[i] = 0;
			}
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
			for(int i = 0; i < 5; i++) {
				Local::SemipersistentData::auto_ladeleistung_log_in_w[i] = 0;
			}
		}

//		void starte_router_neu() {
//			_log((char*) "starte_router_neu");
//			_schalte_shellyplug(
//				false, cfg->router_neustart_relay_host, cfg->router_neustart_relay_port,
//				web_reader->default_timeout_in_hundertstel_s
//			);
//			// Schaltet allein wieder ein (killt ja das netz, was ein Einschalten unmöglich macht)
//		}
	};
}
