#pragma once
#include "config.h"
#include "wlan.h"
#include "webserver.h"
#include "web_client.h"
//#include "persistenz.h"
//#include "formatierer.h"
#include "elektro_anlage.h"
//#include "wetter.h"
//#include "smartmeter_leser.h"
#include "wechselrichter_leser.h"
//#include "wettervorhersage_leser.h"
#include <TimeLib.h>

namespace Local {
	class WebPresenter {
	protected:
		Local::Config cfg;

		Local::ElektroAnlage elektroanlage;
//		Local::Wetter wetter;
//		Local::Persistenz persistenz;
		Local::WebClient web_client;

		char int_as_char[16];

		void _print_char_to_web(char* c) {
			webserver.server.sendContent(c);
		}

		void _print_int_to_web(int i) {
			char int_as_char[16];
			itoa(i, int_as_char, 10);
			webserver.server.sendContent((char*) int_as_char);
		}

	public:
		Local::Webserver webserver;

		WebPresenter(
			Local::Config& cfg, Local::Wlan& wlan
		): cfg(cfg), web_client(wlan.client), webserver(cfg.webserver_port) {
		}

		void zeige_hauptseite() {
			// Serial.println(printf("Date: %4d-%02d-%02d %02d:%02d:%02d\n", year(time), month(time), day(time), hour(time), minute(time), second(time)));
			int now_timestamp = webserver.server.arg("time").toInt();
			if(now_timestamp < 1674987010) {
				webserver.server.send(400, "text/plain", "Bitte den aktuellen UnixTimestamp via Parameter 'time' angeben.");
				return;
			}

			Local::WechselrichterLeser wechselrichter_leser(cfg, web_client);
			wechselrichter_leser.daten_holen_und_einsetzen(elektroanlage);

//			Local::SmartmeterLeser smartmeter_leser(cfg, web_client);
//			smartmeter_leser.daten_holen_und_einsetzen(elektroanlage);
//
//			Local::WettervorhersageLeser wetter_leser(cfg, web_client);
//
//			String last_weather_request_timestamp = persistenz.read_file_content((char*) "last_weather_request.txt");
//			if(
//				(
//					last_weather_request_timestamp.toInt() < now_timestamp - 60*45// max alle 45min
//					&& minute(now_timestamp) < 15
//					&& minute(now_timestamp) >= 3// immer kurz nach um, damit die ForecastAPI Zeit hat
//				)
//				|| webserver.server.arg("reset").toInt() == 1 // DEBUG
//			) {// Insgesamt also 1x die Stunde ca 3 nach um
//				Serial.println("Reset");
//				wetter_leser.daten_holen_und_persistieren(persistenz);
//				persistenz.write2file((char*) "last_weather_request.txt", (String) now_timestamp);
//			}
//
//			wetter_leser.persistierte_daten_einsetzen(persistenz, wetter);
//			persistenz.append2file(
//				(char*) "anlage.csv",
//				(String) now_timestamp + ";"
//				+ elektroanlage.gib_log_zeile() + ";"
//				+ wetter.gib_log_zeile()
//			);

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
	//				s((String) wetter.gib_stundenvorhersage_wolkendichte(0));
				_print_char_to_web((char*) "],");
				_print_char_to_web((char*) "\"solarstrahlung\":[");
	//				s((String) wetter.gib_stundenvorhersage_solarstrahlung(0));
				_print_char_to_web((char*) "]");
			_print_char_to_web((char*) "}");
		}
	};
}
