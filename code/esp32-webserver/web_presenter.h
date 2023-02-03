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
		const char* last_weather_request_timestamp_filename = "last_weather_request.csv";
		const char* anlagen_log_filename = "anlage_log.csv";

		void _print_char_to_web(char* c) {
			webserver.server.sendContent(c);
		}

		void _print_int_to_web(int i) {
			char int_as_char[16];
			itoa(i, int_as_char, 10);
			webserver.server.sendContent((char*) int_as_char);
		}

		int _read_last_weather_request_timestamp() {
			int last_weather_request_timestamp = 0;
			if(persistenz.open_file_to_read(last_weather_request_timestamp_filename)) {
				while(persistenz.read_next_block_to_buffer()) {
					if(persistenz.find_in_content((char*) "\nlast_weather_request,([0-9]+),")) {
						last_weather_request_timestamp = atoi(persistenz.finding_buffer);
					}
				}
				persistenz.close_file();
			}
			return last_weather_request_timestamp;
		}

		void _write_last_weather_request_timestamp(int timestamp) {
			if(persistenz.open_file_to_overwrite(last_weather_request_timestamp_filename)) {
				sprintf(persistenz.buffer, "\nlast_weather_request,%d,", timestamp);
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

				persistenz.buffer = ",";
				persistenz.print_buffer_to_file();

				wetter.set_log_data(persistenz.buffer);
				persistenz.print_buffer_to_file();

				persistenz.buffer = "\n";
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

		void zeige_hauptseite() {
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

			int last_weather_request_timestamp = _read_last_weather_request_timestamp();
			Serial.println(last_weather_request_timestamp);
			if(
				(
					last_weather_request_timestamp < now_timestamp - 60*45// max alle 45min
					&& minute(now_timestamp) < 15
					&& minute(now_timestamp) >= 3// immer kurz nach um, damit die ForecastAPI Zeit hat
				)
				|| webserver.server.arg("reset").toInt() == 1 // DEBUG
			) {// Insgesamt also 1x die Stunde ca 3 nach um
				Serial.println("Schreibe wetterdaten");
				// TODO hier weiter! wetter_leser.daten_holen_und_persistieren(persistenz);
				_write_last_weather_request_timestamp(now_timestamp);
			}

			wetter_leser.persistierte_daten_einsetzen(persistenz, wetter);

			_write_log_data(now_timestamp);

			webserver.server.setContentLength(CONTENT_LENGTH_UNKNOWN);
			webserver.server.send(200, "application/json", "");
			_print_char_to_web((char*) "{");

				_print_char_to_web((char*)  "\"max_i_in_ma\":");
					_print_int_to_web(elektroanlage.max_i_in_ma());
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"max_i_phase\":");
					_print_int_to_web(elektroanlage.max_i_phase());
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"ueberschuss_in_wh\":");
					_print_int_to_web(elektroanlage.gib_ueberschuss_in_wh());
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"solarakku_ladestand_in_promille\":");
					_print_int_to_web(elektroanlage.solarakku_ladestand_in_promille);
					_print_char_to_web((char*) ",");

				_print_char_to_web((char*) "\"wolkendichte\":[");
					for(int i = 0; i < 12; i++) {
						_print_int_to_web(wetter.gib_stundenvorhersage_wolkendichte(i));
						if(i != 12) {
							_print_char_to_web((char*) ",");
						}
					}
				_print_char_to_web((char*) "],");

				_print_char_to_web((char*) "\"solarstrahlung\":[");
					for(int i = 0; i < 12; i++) {
						_print_int_to_web(wetter.gib_stundenvorhersage_solarstrahlung(i));
						if(i != 12) {
							_print_char_to_web((char*) ",");
						}
					}
				_print_char_to_web((char*) "]");

			_print_char_to_web((char*) "}");
		}
	};
}
