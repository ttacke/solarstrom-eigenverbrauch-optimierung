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
#include <TimeLib.h>

namespace Local {
	class WebPresenter {
	protected:
	// TODO Pointer zeigen nur auf den Speicher, koennen auch ins Nichts zeigen und unterwegs verändert werden (neue instanz und so)
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

				sprintf(persistenz.buffer, "prv1,%d,%d",
					stunden_wettervorhersage_letzter_abruf,
					tages_wettervorhersage_letzter_abruf
				);
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
					// TODO am Ende steht ein "d", wieso?
					_print_char_to_web(persistenz.buffer);
				}
				persistenz.close_file();
			} else {
				_print_char_to_web((char*) "<h1>Bitte die code/htdocs/index.html auf die SD-Karte im root-Ordner ablegen</h1>");
			}
		}

		void zeige_log() {
			webserver.server.setContentLength(CONTENT_LENGTH_UNKNOWN);
			webserver.server.send(200, "text/plain", "");
			if(persistenz.open_file_to_read(anlagen_log_filename)) {
				while(persistenz.read_next_block_to_buffer()) {
					_print_char_to_web(persistenz.buffer);
				}
				persistenz.close_file();
			} else {
				_print_char_to_web((char*) "Keine Daten!");
			}
		}

		void zeige_daten() {
			// Serial.println(printf("Date: %4d-%02d-%02d %02d:%02d:%02d\n", year(time), month(time), day(time), hour(time), minute(time), second(time)));
			int now_timestamp = webserver.server.arg("time").toInt();
			if(now_timestamp < 1674987010) {
				webserver.server.send(400, "text/plain", "Bitte den aktuellen UnixTimestamp via Parameter 'time' angeben.");
				return;
			}

			Local::WechselrichterLeser wechselrichter_leser(*cfg, web_client);
			wechselrichter_leser.daten_holen_und_einsetzen(elektroanlage);

			Local::SmartmeterLeser smartmeter_leser(*cfg, web_client);
			smartmeter_leser.daten_holen_und_einsetzen(elektroanlage);

			Local::WettervorhersageLeser wetter_leser(*cfg, web_client);

			_lese_systemstatus_daten();
			if(
				!stunden_wettervorhersage_letzter_abruf
				|| (
					stunden_wettervorhersage_letzter_abruf < now_timestamp - 60*45// max alle 45min
					&& minute(now_timestamp) < 15
					&& minute(now_timestamp) >= 3// immer kurz nach um, damit die ForecastAPI Zeit hat
				)
				|| webserver.server.arg("reset").toInt() == 1 // DEBUG
			) {// Insgesamt also 1x die Stunde ca 3 nach um
				Serial.println("Schreibe Stunden-Wettervorhersage");
				wetter_leser.stundendaten_holen_und_persistieren(persistenz);
				stunden_wettervorhersage_letzter_abruf = now_timestamp;
				_schreibe_systemstatus_daten();
			}

			if(
				!tages_wettervorhersage_letzter_abruf
				|| (
					tages_wettervorhersage_letzter_abruf < now_timestamp - (3600*5 + 60*45)// max alle 5:45h
					&& minute(now_timestamp) < 25
					&& minute(now_timestamp) >= 15// immer kurz nach um, damit die ForecastAPI Zeit hat
				)
				|| webserver.server.arg("reset").toInt() == 1 // DEBUG
			) {// Insgesamt also 1x alle 6 Stunden ca 15 nach um
				Serial.println("Schreibe Tages-Wettervorhersage");
				wetter_leser.tagesdaten_holen_und_persistieren(persistenz);
				tages_wettervorhersage_letzter_abruf = now_timestamp;
				_schreibe_systemstatus_daten();
			}

			wetter_leser.persistierte_daten_einsetzen(persistenz, wetter);

			_write_log_data(now_timestamp);

			webserver.server.setContentLength(CONTENT_LENGTH_UNKNOWN);
			webserver.server.send(200, "application/json", "");
			_print_char_to_web((char*) "{");

				_print_char_to_web((char*) "\"ueberschuss_in_wh\":");
					_print_int_to_web(elektroanlage.gib_ueberschuss_in_wh());
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"solarakku_ladestand_in_promille\":");
					_print_int_to_web(elektroanlage.solarakku_ladestand_in_promille);
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"wolkendichte\":[");
					for(int i = 0; i < 12; i++) {
						_print_int_to_web(wetter.gib_stundenvorhersage_wolkendichte(i));
						if(i != 11) {
							_print_char_to_web((char*) ",");
						}
					}
				_print_char_to_web((char*) "],");

				_print_char_to_web((char*) "\"solarstrahlung_stunden_startzeit\":");
					_print_int_to_web(stunden_wettervorhersage_letzter_abruf);
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"solarstrahlung_stunden\":[");
					for(int i = 0; i < 12; i++) {
						_print_int_to_web(wetter.gib_stundenvorhersage_solarstrahlung(i));
						if(i != 11) {
							_print_char_to_web((char*) ",");
						}
					}
				_print_char_to_web((char*) "],");

				_print_char_to_web((char*) "\"solarstrahlung_tage_startzeit\":");
					_print_int_to_web(tages_wettervorhersage_letzter_abruf);
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"solarstrahlung_tage\":[");
					for(int i = 0; i < 5; i++) {
						_print_int_to_web(wetter.gib_tagesvorhersage_solarstrahlung(i));
						if(i != 4) {
							_print_char_to_web((char*) ",");
						}
					}
				_print_char_to_web((char*) "]");

			_print_char_to_web((char*) "}");
		}
	};
}
