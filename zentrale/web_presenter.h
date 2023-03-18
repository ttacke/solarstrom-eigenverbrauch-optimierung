#pragma once
#include "config.h"
#include "wlan.h"
#include "webserver.h"
#include "web_client.h"
#include "persistenz.h"
#include "elektro_anlage.h"
#include "wetter.h"
#include "smartmeter_api.h"
#include "wechselrichter_api.h"
#include "wettervorhersage_api.h"
#include "verbraucher.h"
#include "verbraucher_api.h"
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
		Local::Verbraucher verbraucher;

		char int_as_char[16];
		char output_buffer[32];
		// TODO configwerte als SD_File ablegen? dann können komlpexere Dinge außerhalb laufen
		// z.b. die Grundlast und die Monatlichen/Stündlichen Anpassungen für Strahlungswerte
		const char* system_status_filename = "system_status.csv";
		int stunden_wettervorhersage_letzter_abruf;
		int tages_wettervorhersage_letzter_abruf;
		const char* anlagen_log_filename = "anlage_log.csv";
		const char* ui_filename = "index.html";

		void _print_char_to_web(char* c) {
			webserver.server.sendContent(c);
		}

		void _lese_systemstatus_daten() {
			if(persistenz.open_file_to_read(system_status_filename)) {
				while(persistenz.read_next_block_to_buffer()) {
					if(persistenz.find_in_buffer((char*) "\nstunden_wettervorhersage_letzter_abruf,([0-9]+),")) {
						stunden_wettervorhersage_letzter_abruf = atoi(persistenz.finding_buffer);
					}
					if(persistenz.find_in_buffer((char*) "\ntages_wettervorhersage_letzter_abruf,([0-9]+),")) {
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

				sprintf(persistenz.buffer, "%s", ",");
				persistenz.print_buffer_to_file();

				wetter.set_log_data(persistenz.buffer);
				persistenz.print_buffer_to_file();

				sprintf(persistenz.buffer, "%s", ",");
				persistenz.print_buffer_to_file();

				verbraucher.set_log_data_a(persistenz.buffer);
				persistenz.print_buffer_to_file();

				sprintf(persistenz.buffer, "%s", ",");
				persistenz.print_buffer_to_file();

				verbraucher.set_log_data_b(persistenz.buffer);
				persistenz.print_buffer_to_file();

				sprintf(persistenz.buffer, "%s", "\n");
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

		void download_file(const char* filename) {
			if(persistenz.open_file_to_read(filename)) {
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
			Local::VerbraucherAPI verbraucher_api(*cfg, web_client, persistenz);
			int now_timestamp = verbraucher_api.timestamp;
			if(!now_timestamp) {
				now_timestamp = webserver.server.arg("time").toInt();
				if(now_timestamp < 1674987010) {
					webserver.server.send(400, "text/plain", "Bitte den aktuellen UnixTimestamp via Parameter 'time' angeben.");
					return;
				}
			}
			const char* key = webserver.server.arg("key").c_str();
			const char* val = webserver.server.arg("val").c_str();

			if(strcmp(key, "auto") == 0) {
				if(strcmp(val, "force") == 0) {
					verbraucher_api.setze_auto_ladestatus(Local::Verbraucher::Ladestatus::force);
				} else if(strcmp(val, "solar") == 0) {
					verbraucher_api.setze_auto_ladestatus(Local::Verbraucher::Ladestatus::solar);
				} else if(strcmp(val, "change_power") == 0) {
					verbraucher_api.wechsle_auto_ladeleistung();
				}
			} else if(strcmp(key, "roller") == 0) {
				if(strcmp(val, "force") == 0) {
					verbraucher_api.setze_roller_ladestatus(Local::Verbraucher::Ladestatus::force);
				} else if(strcmp(val, "solar") == 0) {
					verbraucher_api.setze_roller_ladestatus(Local::Verbraucher::Ladestatus::solar);
				} else if(strcmp(val, "change_power") == 0) {
					verbraucher_api.wechsle_roller_ladeleistung();
				}
			}
			webserver.server.send(204, "text/plain", "");
		}

		void heartbeat(const char* data_filename) {
			// Serial.println(printf("Date: %4d-%02d-%02d %02d:%02d:%02d\n", year(time), month(time), day(time), hour(time), minute(time), second(time)));
			Local::VerbraucherAPI verbraucher_api(*cfg, web_client, persistenz);
			int now_timestamp = verbraucher_api.timestamp;
			if(!now_timestamp) {
				now_timestamp = webserver.server.arg("time").toInt();
				if(now_timestamp < 1674987010) {
					webserver.server.send(400, "text/plain", "Bitte den aktuellen UnixTimestamp via Parameter 'time' angeben.");
					return;
				}
			}

			Local::WechselrichterAPI wechselrichter_api(*cfg, web_client);
			wechselrichter_api.daten_holen_und_einsetzen(elektroanlage);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			Local::SmartmeterAPI smartmeter_api(*cfg, web_client);
			smartmeter_api.daten_holen_und_einsetzen(elektroanlage);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			Local::WettervorhersageAPI wettervorhersage_api(*cfg, web_client);

			_lese_systemstatus_daten();
			if(
				!stunden_wettervorhersage_letzter_abruf
				|| (
					stunden_wettervorhersage_letzter_abruf < now_timestamp - 60*45// max alle 45min
					&& minute(now_timestamp) < 20
					&& minute(now_timestamp) >= 10
				)
			) {// Insgesamt also 1x die Stunde ca 10 nach um
				Serial.println("Schreibe Stunden-Wettervorhersage");
				wettervorhersage_api.stundendaten_holen_und_persistieren(persistenz);
				stunden_wettervorhersage_letzter_abruf = now_timestamp;
				_schreibe_systemstatus_daten();
				yield();
			}

			if(
				!tages_wettervorhersage_letzter_abruf
				|| (
					tages_wettervorhersage_letzter_abruf < now_timestamp - (3600*4)
				)
			) {// Insgesamt also 1x alle 4 Stunden
				Serial.println("Schreibe Tages-Wettervorhersage");
				wettervorhersage_api.tagesdaten_holen_und_persistieren(persistenz);
				tages_wettervorhersage_letzter_abruf = now_timestamp;
				_schreibe_systemstatus_daten();
				yield();
			}
			wettervorhersage_api.persistierte_daten_einsetzen(persistenz, wetter, now_timestamp);

			verbraucher_api.daten_holen_und_einsetzen(verbraucher, elektroanlage, wetter);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher_api.fuehre_schaltautomat_aus(verbraucher);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			_write_log_data(now_timestamp);

			if(persistenz.open_file_to_overwrite(data_filename)) {
				int anteil_pv1_in_prozent = elektroanlage.gib_anteil_pv1_in_prozent();
				sprintf(persistenz.buffer,
					"{\"ueberschuss_in_wh\":%i,\"solarakku_ist_an\":%s,",
					elektroanlage.gib_ueberschuss_in_w(),
					(elektroanlage.solarakku_ist_an ? "true" : "false")
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"solarakku_ladestand_in_promille\":%i,",
					elektroanlage.solarakku_ladestand_in_promille
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"solaranteil_in_prozent_string1\":%i,",
					anteil_pv1_in_prozent
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"solaranteil_in_prozent_string2\":%i,",
					100 - anteil_pv1_in_prozent
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"stunden_balkenanzeige_startzeit\":%i,",
					wetter.stundenvorhersage_startzeitpunkt
				);
				persistenz.print_buffer_to_file();
				int stundenvorhersage[12];
				for(int i = 0; i < 12; i++) {
					stundenvorhersage[i] = verbraucher.gib_stundenvorhersage_akku_ladestand_als_fibonacci(i);
				}
				sprintf(persistenz.buffer,
					"\"stunden_balkenanzeige\":"
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"[%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i],",
					stundenvorhersage[0],
					stundenvorhersage[1],
					stundenvorhersage[2],
					stundenvorhersage[3],
					stundenvorhersage[4],
					stundenvorhersage[5],
					stundenvorhersage[6],
					stundenvorhersage[7],
					stundenvorhersage[8],
					stundenvorhersage[9],
					stundenvorhersage[10],
					stundenvorhersage[11]
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"tage_balkenanzeige_startzeit\":%i,",
					wetter.tagesvorhersage_startzeitpunkt
				);
				persistenz.print_buffer_to_file();
				int tagesvorhersage[5];
				for(int i = 0; i < 5; i++) {
					tagesvorhersage[i] = wetter.gib_tagesvorhersage_solarstrahlung_als_fibonacci(i);
				}
				sprintf(persistenz.buffer,
					"\"tage_balkenanzeige\":[%i,%i,%i,%i,%i],",
					tagesvorhersage[0],
					tagesvorhersage[1],
					tagesvorhersage[2],
					tagesvorhersage[3],
					tagesvorhersage[4]
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"auto_laden_an\":%s,\"roller_laden_an\":%s,",
					verbraucher.auto_laden_ist_an() ? "true" : "false",
					verbraucher.roller_laden_ist_an() ? "true" : "false"
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"wasser_ueberladen\":%s,\"heizung_ueberladen\":%s,",
					verbraucher.wasser_relay_ist_an ? "true" : "false",
					verbraucher.heizung_relay_ist_an ? "true" : "false"
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"auto_laden\":%s,\"roller_laden\":%s,",
					verbraucher.auto_ladestatus == Local::Verbraucher::Ladestatus::force
						? "\"force\"" : "\"solar\"",
					verbraucher.roller_ladestatus == Local::Verbraucher::Ladestatus::force
						? "\"force\"" : "\"solar\""
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"auto_benoetigte_ladeleistung_in_w\":%i,",
					verbraucher.auto_benoetigte_ladeleistung_in_w
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"roller_benoetigte_ladeleistung_in_w\":%i,",
					verbraucher.roller_benoetigte_ladeleistung_in_w
				);
				persistenz.print_buffer_to_file();
				sprintf(persistenz.buffer,
					"\"solarerzeugung_ist_aktiv\":%s,\"timestamp\":%i}",
					verbraucher.solarerzeugung_ist_aktiv() ? "true" : "false",
					now_timestamp
				);
				persistenz.print_buffer_to_file();
				persistenz.close_file();
			}
		}
	};
}
