#pragma once
#include "config.h"
#include "wlan.h"
#include "webserver.h"
#include "web_client.h"
#include "persistenz.h"
#include "elektro_anlage.h"
#include "wetter.h"
#include "smartmeter_leser.h"
#include "wechselrichter_leser.h"
#include "wettervorhersage_leser.h"
#include "wasser_netz_relay_api.h"
#include "heizungs_netz_relay_api.h"
#include "auto_netz_relay_api.h"
#include "roller_netz_relay_api.h"
#include <TimeLib.h>

namespace Local {
	class WebPresenter {
	protected:
	// Pointer zeigen nur auf den Speicher, koennen auch ins Nichts zeigen und unterwegs verändert werden (neue instanz und so)
	// Referenzen sind mit RefCount verbunden und unveränderlich
	// Wieso knallts dann bei referenzen? Wird eins der Objekte zerstört? Muss man Referenzen überall mit & notieren?
		Local::Config* cfg;
		Local::WebClient web_client;
		Local::Persistenz persistenz;

		Local::ElektroAnlage elektroanlage;
		Local::Wetter wetter;

		char int_as_char[16];
		const char* system_status_filename = "system_status.csv";
		int stunden_wettervorhersage_letzter_abruf;
		int tages_wettervorhersage_letzter_abruf;
		const char* anlagen_log_filename = "anlage_log.csv";
		const char* ui_filename = "index.html";

		void _print_char_to_web(char* c) {
			webserver.server.sendContent(c);
		}

		void _print_int_to_web(int i) {
			char int_as_char[16];
			itoa(i, int_as_char, 10);
			webserver.server.sendContent((char*) int_as_char);
		}

		void _lese_systemstatus_daten() {
			if(persistenz.open_file_to_read(system_status_filename)) {
				while(persistenz.read_next_block_to_buffer()) {
					if(persistenz.find_in_content((char*) "\nstunden_wettervorhersage_letzter_abruf,([0-9]+),")) {
						stunden_wettervorhersage_letzter_abruf = atoi(persistenz.finding_buffer);
					}
					if(persistenz.find_in_content((char*) "\ntages_wettervorhersage_letzter_abruf,([0-9]+),")) {
						tages_wettervorhersage_letzter_abruf = atoi(persistenz.finding_buffer);
					}
				}
				persistenz.close_file();
			}
		}

		void _schreibe_systemstatus_daten() {
			if(persistenz.open_file_to_overwrite(system_status_filename)) {
				sprintf(persistenz.buffer, "\nstunden_wettervorhersage_letzter_abruf,%d,", stunden_wettervorhersage_letzter_abruf);
				persistenz.print_buffer_to_file();

				sprintf(persistenz.buffer, "\ntages_wettervorhersage_letzter_abruf,%d,", tages_wettervorhersage_letzter_abruf);
				persistenz.print_buffer_to_file();

				persistenz.close_file();
			}
		}

		void _write_log_data(int now_timestamp) {
			if(persistenz.open_file_to_append(anlagen_log_filename)) {
				sprintf(persistenz.buffer, "%d,", now_timestamp);
				persistenz.print_buffer_to_file();

				elektroanlage.set_log_data(persistenz.buffer);
				persistenz.print_buffer_to_file();

				memcpy(persistenz.buffer, ",\0", 2);
				persistenz.print_buffer_to_file();

				wetter.set_log_data(persistenz.buffer);
				persistenz.print_buffer_to_file();

				memcpy(persistenz.buffer, "\n\0", 2);
				persistenz.print_buffer_to_file();

				persistenz.close_file();
			}
		}

	public:
		Local::Webserver webserver;

		WebPresenter(
			Local::Config& cfg, Local::Wlan& wlan
		): cfg(&cfg), web_client(wlan.client), webserver(cfg.webserver_port) {
		}

		void zeige_ui() {
			webserver.server.setContentLength(CONTENT_LENGTH_UNKNOWN);
			webserver.server.send(200, "text/html", "");
			if(persistenz.open_file_to_read(ui_filename)) {
				while(persistenz.read_next_block_to_buffer()) {
					_print_char_to_web(persistenz.buffer);
				}
				persistenz.close_file();
			} else {
				_print_char_to_web((char*) "<h1>Bitte im Projekt 'cd code/scripte;perl schreibe_indexdatei.pl [IP]' ausf&uuml;hren</h1>");
			}
		}

		void download_file() {
			if(persistenz.open_file_to_read((const char*) webserver.server.arg("name").c_str())) {
				webserver.server.setContentLength(CONTENT_LENGTH_UNKNOWN);
				webserver.server.send(200, "text/plain", "");
				while(persistenz.read_next_block_to_buffer()) {
					_print_char_to_web(persistenz.buffer);

					yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
				}
				persistenz.close_file();
			} else {
				webserver.server.send(404, "text/plain", "Not found");
			}
		}

		void upload_file() {
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			HTTPUpload& upload = webserver.server.upload();
			const char* filename = webserver.server.arg("name").c_str();
			if(upload.status == UPLOAD_FILE_START) {
				if(persistenz.open_file_to_overwrite(filename)) {
					persistenz.close_file();
				} else {
					webserver.server.send(500, "text/plain", "Kann Datei nicht schreiben");
				}
			} else if(upload.status == UPLOAD_FILE_WRITE) {
				if(persistenz.open_file_to_append(filename)) {
					int offset = 0;
					while(true) {
						int read_size = std::min(sizeof(persistenz.buffer), upload.currentSize - offset);
						if(read_size <= 0) {
							break;
						} else {
							memcpy(persistenz.buffer, upload.buf + offset, read_size);
							if(read_size < sizeof(persistenz.buffer)) {
								std::fill(persistenz.buffer + read_size, persistenz.buffer + sizeof(persistenz.buffer), 0);
							}
							persistenz.print_buffer_to_file();
							offset += read_size;
						}
					}
					persistenz.close_file();
				}
			  } else if(upload.status == UPLOAD_FILE_END){
				  webserver.server.send(201);
			  }
		}

		void aendere() {
			int now_timestamp = webserver.server.arg("time").toInt();
			if(now_timestamp < 1674987010) {
				webserver.server.send(400, "text/plain", "Bitte den aktuellen UnixTimestamp via Parameter 'time' angeben.");
				return;
			}
			const char* key = webserver.server.arg("key").c_str();
			const char* val = webserver.server.arg("val").c_str();

			if(strcmp(key, "auto") == 0) {
				Local::AutoNetzRelayAPI auto_netz_relay_api(*cfg, web_client);
				auto_netz_relay_api.fuehre_aus(val, now_timestamp);
			} else if(strcmp(key, "roller") == 0) {
				Local::RollerNetzRelayAPI roller_netz_relay_api(*cfg, web_client);
				roller_netz_relay_api.fuehre_aus(val, now_timestamp);
			}
			webserver.server.send(204, "text/plain", "");
		}

		void zeige_daten(bool erneuere_daten_automatisch) {
			// Serial.println(printf("Date: %4d-%02d-%02d %02d:%02d:%02d\n", year(time), month(time), day(time), hour(time), minute(time), second(time)));
			int now_timestamp = webserver.server.arg("time").toInt();
			if(now_timestamp < 1674987010) {
				webserver.server.send(400, "text/plain", "Bitte den aktuellen UnixTimestamp via Parameter 'time' angeben.");
				return;
			}

			Local::WasserNetzRelayAPI wasser_netz_relay_api(*cfg, web_client);
			wasser_netz_relay_api.heartbeat(now_timestamp);
			Local::HeizungsNetzRelayAPI heizungs_netz_relay_api(*cfg, web_client);
			heizungs_netz_relay_api.heartbeat(now_timestamp);
			Local::AutoNetzRelayAPI auto_netz_relay_api(*cfg, web_client);
			auto_netz_relay_api.heartbeat(now_timestamp);
			Local::RollerNetzRelayAPI roller_netz_relay_api(*cfg, web_client);
			roller_netz_relay_api.heartbeat(now_timestamp);
			// TODO
			// Relays prüfen: sollte dort das delay() raus?
			// ShellyPlug: zeit von dort holen? Neee...

			Local::WechselrichterLeser wechselrichter_leser(*cfg, web_client);
			wechselrichter_leser.daten_holen_und_einsetzen(elektroanlage);

			Local::SmartmeterLeser smartmeter_leser(*cfg, web_client);
			smartmeter_leser.daten_holen_und_einsetzen(elektroanlage);

			Local::WettervorhersageLeser wetter_leser(*cfg, web_client);

			_lese_systemstatus_daten();
			if(erneuere_daten_automatisch) {
				if(
					!stunden_wettervorhersage_letzter_abruf
					|| (
						stunden_wettervorhersage_letzter_abruf < now_timestamp - 60*45// max alle 45min
						&& minute(now_timestamp) < 20
						&& minute(now_timestamp) >= 10
					)
				) {// Insgesamt also 1x die Stunde ca 10 nach um
					Serial.println("Schreibe Stunden-Wettervorhersage");
					wetter_leser.stundendaten_holen_und_persistieren(persistenz);
					stunden_wettervorhersage_letzter_abruf = now_timestamp;
					_schreibe_systemstatus_daten();
				}

				if(
					!tages_wettervorhersage_letzter_abruf
					|| (
						tages_wettervorhersage_letzter_abruf < now_timestamp - (3600*4)
					)
				) {// Insgesamt also 1x alle 4 Stunden
					Serial.println("Schreibe Tages-Wettervorhersage");
					wetter_leser.tagesdaten_holen_und_persistieren(persistenz);
					tages_wettervorhersage_letzter_abruf = now_timestamp;
					_schreibe_systemstatus_daten();
				}
			}
			wetter_leser.persistierte_daten_einsetzen(persistenz, wetter, now_timestamp);

			if(erneuere_daten_automatisch) {
				_write_log_data(now_timestamp);
			}

			webserver.server.setContentLength(CONTENT_LENGTH_UNKNOWN);
			webserver.server.send(200, "application/json", "");
			_print_char_to_web((char*) "{");

				_print_char_to_web((char*) "\"ueberschuss_in_wh\":");
					_print_int_to_web(elektroanlage.gib_ueberschuss_in_w());
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"solarakku_ist_an\":");
					_print_char_to_web((char*) (elektroanlage.solarakku_ist_an ? "true" : "false"));
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"solarakku_ladestand_in_promille\":");
					_print_int_to_web(elektroanlage.solarakku_ladestand_in_promille);
					_print_char_to_web((char*) ",");

				int anteil_pv1_in_prozent = elektroanlage.gib_anteil_pv1_in_prozent();
				_print_char_to_web((char*) "\"solaranteil_in_prozent_string1\":");
					_print_int_to_web(anteil_pv1_in_prozent);
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"solaranteil_in_prozent_string2\":");
					_print_int_to_web(100 - anteil_pv1_in_prozent);
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"solarstrahlung_stunden_startzeit\":");
					_print_int_to_web(wetter.stundenvorhersage_startzeitpunkt);
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"solarstrahlung_stunden_in_prozent\":[");
					for(int i = 0; i < 12; i++) {
						_print_int_to_web(wetter.gib_stundenvorhersage_solarstrahlung_in_prozent(i, *cfg));
						if(i != 11) {
							_print_char_to_web((char*) ",");
						}
					}
				_print_char_to_web((char*) "],");

				_print_char_to_web((char*) "\"solarstrahlung_tage_startzeit\":");
					_print_int_to_web(wetter.tagesvorhersage_startzeitpunkt);
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"solarstrahlung_tage_in_prozent\":[");
					for(int i = 0; i < 5; i++) {
						_print_int_to_web(wetter.gib_tagesvorhersage_solarstrahlung_in_prozent(i, *cfg));
						if(i != 4) {
							_print_char_to_web((char*) ",");
						}
					}
				_print_char_to_web((char*) "],");

				_print_char_to_web((char*) "\"wasser_ueberladen\":");
					_print_char_to_web((char*) (wasser_netz_relay_api.ist_aktiv() ? "true" : "false"));
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"heizung_ueberladen\":");
					_print_char_to_web((char*) (heizungs_netz_relay_api.ist_aktiv() ? "true" : "false"));
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"auto_laden\":");
					_print_char_to_web((char*) (
						auto_netz_relay_api.ist_force_aktiv()
						? "\"force\""
						: auto_netz_relay_api.ist_solar_aktiv()
							? "\"solar\""
							: "\"off\""
					));
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"auto_ladeleistung_in_w\":");
					_print_int_to_web(auto_netz_relay_api.gib_aktuelle_ladeleistung_in_w());
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"roller_laden\":");
					_print_char_to_web((char*) (
						roller_netz_relay_api.ist_force_aktiv()
						? "\"force\""
						: roller_netz_relay_api.ist_solar_aktiv()
							? "\"solar\""
							: "\"off\""
					));
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"roller_ladeleistung_in_w\":");
					_print_int_to_web(roller_netz_relay_api.gib_aktuelle_ladeleistung_in_w());

			_print_char_to_web((char*) "}");
		}
	};
}
