#pragma once
#include "config.h"
#include "wlan.h"
#include "webserver.h"
#include "web_client.h"
#include "persistenz.h"
#include "formatierer.h"
#include "elektro_anlage.h"
#include "wetter.h"
#include "smartmeter_leser.h"
#include "wechselrichter_leser.h"
#include "wettervorhersage_leser.h"
#include <TimeLib.h>

namespace Local {
	class WebPresenter {
	protected:
		Local::Config cfg;

		Local::ElektroAnlage elektroanlage;
		Local::Wetter wetter;
		Local::Persistenz persistenz;
		Local::WebClient web_client;

		void s(String content) {// TODO besser von SD laden!
			webserver.server.sendContent(content);
		}

	public:
		Local::Webserver webserver;

		WebPresenter(
			Local::Config& cfg, Local::Wlan& wlan
		): cfg(cfg), web_client(wlan.client), webserver(cfg.webserver_port) {
		}

		void zeige_hauptseite() {
			int now_timestamp = webserver.server.arg("time").toInt();

			Local::WechselrichterLeser wechselrichter_leser(cfg, web_client);
			wechselrichter_leser.daten_holen_und_einsetzen(elektroanlage);

			Local::SmartmeterLeser smartmeter_leser(cfg, web_client);
			smartmeter_leser.daten_holen_und_einsetzen(elektroanlage);

			Local::WettervorhersageLeser wetter_leser(cfg, web_client);

			if(now_timestamp > 1674987010) {// Gueltiger timestamp noetig
				Serial.println("Time vorhanden");// TODO das muss beim Daten lesen immer da sein
				// Serial.println(printf("Date: %4d-%02d-%02d %02d:%02d:%02d\n", year(time), month(time), day(time), hour(time), minute(time), second(time)));
				String last_weather_request_timestamp = persistenz.read_file_content((char*) "last_weather_request.txt");
				if(
					(
						last_weather_request_timestamp.toInt() < now_timestamp - 60*45// max alle 45min
						&& minute(now_timestamp) < 15
						&& minute(now_timestamp) >= 3// immer kurz nach um, damit die ForecastAPI Zeit hat
					)
					|| webserver.server.arg("reset").toInt() == 1 // DEBUG
				) {// Insgesamt also 1x die Stunde ca 3 nach um
					Serial.println("Reset");
					wetter_leser.daten_holen_und_persistieren(persistenz);
					persistenz.write2file((char*) "last_weather_request.txt", (String) now_timestamp);
				}

				wetter_leser.persistierte_daten_einsetzen(persistenz, wetter);
				persistenz.append2file(
					(char*) "anlage.csv",
					(String) now_timestamp + ";"
					+ elektroanlage.gib_log_zeile() + ";"
					+ wetter.gib_log_zeile()
				);
			}

			webserver.server.setContentLength(CONTENT_LENGTH_UNKNOWN);
			webserver.server.send(200, "application/json", "");
			s((String) "{");
			s((String) "\"max_i_in_ma\":" + (String) elektroanlage.max_i_in_ma() + (String) ",");
			s((String) "\"max_i_phase\":" + elektroanlage.max_i_phase() + (String) ",");
			s((String) "\"ueberschuss_in_wh\":" + (String) elektroanlage.gib_ueberschuss_in_wh() + (String) ",");
			s((String) "\"solarakku_ladestand_in_promille\":" + (String) elektroanlage.solarakku_ladestand_in_promille + (String) ",");
			s((String) "\"wolkendichte\":[");
				s((String) wetter.gib_stundenvorhersage_wolkendichte(0));
			s((String) "],");
			s((String) "\"solarstrahlung\":[");
				s((String) wetter.gib_stundenvorhersage_solarstrahlung(0));
			s((String) "]");
			s((String) "}");
		}
	};
}
