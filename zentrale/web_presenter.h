#pragma once
#include "config.h"
#include "service/wlan.h"
#include "service/webserver.h"
#include "service/web_reader.h"
#include "service/file_reader.h"
#include "service/file_writer.h"
#include "model/elektro_anlage.h"
#include "model/wetter.h"
#include "api/smartmeter_api.h"
#include "api/wechselrichter_api.h"
#include "api/wettervorhersage_api.h"
#include "model/verbraucher.h"
#include "api/verbraucher_api.h"
#include "service/web_writer.h"
#include <TimeLib.h>

namespace Local {
	class WebPresenter {
	protected:
	// Pointer zeigen nur auf den Speicher, koennen auch ins Nichts zeigen und unterwegs verändert werden (neue instanz und so)
	// Referenzen sind mit RefCount verbunden und unveränderlich
	// Wieso knallts dann bei referenzen? Wird eins der Objekte zerstört? Muss man Referenzen überall mit & notieren?
		Local::Config* cfg;
		Local::Service::WebReader web_reader;
		Local::Service::WebWriter web_writer;
		Local::Service::FileReader file_reader;
		Local::Service::FileWriter file_writer;

		Local::Model::ElektroAnlage elektroanlage;
		Local::Model::Wetter wetter;
		Local::Model::Verbraucher verbraucher;

		const char* anlagen_log_filename_template = "anlage_log-%4d-%02d.csv";
		const char* ui_filename = "index.html";
		char log_buffer[64];
		float temperature = 0;
		float humidity = 0;
		float cellar_temperature = 0;
		float cellar_humidity = 0;
		int heat_difference = 0;

		void _write_log_data(int now_timestamp, bool heizstabbetrieb_ist_erlaubt) {
			char filename[32];
			sprintf(filename, anlagen_log_filename_template, year(now_timestamp), month(now_timestamp));
			if(file_writer.open_file_to_append(filename)) {
				file_writer.write_formated("%d,", now_timestamp);
				elektroanlage.write_log_data(file_writer);
				yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
				file_writer.write_formated(",");
				wetter.write_log_data(file_writer);
				yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
				file_writer.write_formated(",");
				verbraucher.write_log_data(file_writer);
				yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
				file_writer.write_formated(",kb1%.1f,%.1f", temperature, humidity);
				file_writer.write_formated(",kl1%.1f,%.1f", cellar_temperature, cellar_humidity);
				file_writer.write_formated(",hs%d,%d", (heizstabbetrieb_ist_erlaubt ? 1 : 0), heat_difference);
				file_writer.write_formated("\n");
				file_writer.close_file();
			}
		}

	public:
		Local::Service::Webserver webserver;

		WebPresenter(
			Local::Config& cfg, Local::Service::Wlan& wlan
		):
			cfg(&cfg), web_reader(wlan.client), webserver(cfg.webserver_port),
			web_writer(webserver)
		{}

		void zeige_ui() {
			if(file_reader.open_file_to_read(ui_filename)) {
				web_writer.init_for_write("text/html", file_reader.get_file_size());
				while(file_reader.read_next_block_to_buffer()) {
					web_writer.write(file_reader.buffer, strlen(file_reader.buffer));
				}
				file_reader.close_file();
			} else {
				char* text = (char*) "<h1>Bitte im Projekt 'cd code/scripte;perl schreibe_indexdatei.pl [IP]' ausf&uuml;hren</h1>";
				web_writer.init_for_write("text/html", strlen(text));
				web_writer.write(text, strlen(text));
			}
			web_writer.flush_write_buffer_and_close_transfer();
		}

		void download_file(const char* filename) {
			if(file_reader.open_file_to_read(filename)) {
				web_writer.init_for_write("text/plain", file_reader.get_file_size());
				while(file_reader.read_next_block_to_buffer()) {
					web_writer.write(file_reader.buffer, strlen(file_reader.buffer));
				}
				web_writer.flush_write_buffer_and_close_transfer();
				file_reader.close_file();
			} else {
				webserver.server.send(404, "text/plain", "Not found");
			}
		}

		void set_temperature_and_humidity(float temp, float hum) {
		    temperature = temp;
		    humidity = hum;
		    webserver.server.send(200, "text/plain", "ok");
		}

		void set_cellar_temperature_and_humidity(float temp, float hum) {
		    cellar_temperature = temp;
		    cellar_humidity = hum;
		    webserver.server.send(200, "text/plain", "ok");
		}

		void set_heat_difference(int val) {
		    heat_difference = val;
		    webserver.server.send(200, "text/plain", "ok");
		}

		void upload_file() {
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			HTTPUpload& upload = webserver.server.upload();
			const char* filename = webserver.server.arg("name").c_str();
			if(upload.status == UPLOAD_FILE_START) {
				if(file_writer.open_file_to_overwrite(filename)) {
					file_writer.close_file();
				} else {
					webserver.server.send(500, "text/plain", "Kann Datei nicht schreiben");
				}
			} else if(upload.status == UPLOAD_FILE_WRITE) {
				if(file_writer.open_file_to_append(filename)) {
					file_writer.write(upload.buf, upload.currentSize);
					file_writer.close_file();
				}
			  } else if(upload.status == UPLOAD_FILE_END){
				  webserver.server.send(201);
			  }
		}

		void aendere() {
			Local::Api::VerbraucherAPI verbraucher_api(*cfg, web_reader, file_reader, file_writer, 0);
			int now_timestamp = verbraucher_api.timestamp;
			if(!now_timestamp) {
				webserver.server.send(400, "text/plain", "Der timestamp konnte im System nicht korrekt gelesen werden");
				return;
			}
			const char* key = webserver.server.arg("key").c_str();
			const char* val = webserver.server.arg("val").c_str();

			if(strcmp(key, "auto") == 0) {
				if(strcmp(val, "force") == 0) {
					verbraucher_api.setze_auto_ladestatus(Local::Model::Verbraucher::Ladestatus::force);
				} else if(strcmp(val, "solar") == 0) {
					verbraucher_api.setze_auto_ladestatus(Local::Model::Verbraucher::Ladestatus::solar);
				}
			} else if(strcmp(key, "roller") == 0) {
				if(strcmp(val, "force") == 0) {
					verbraucher_api.setze_roller_ladestatus(Local::Model::Verbraucher::Ladestatus::force);
				} else if(strcmp(val, "solar") == 0) {
					verbraucher_api.setze_roller_ladestatus(Local::Model::Verbraucher::Ladestatus::solar);
				}
			}
			webserver.server.send(204, "text/plain", "");
		}

//		void restart_router() {
//			Local::Api::VerbraucherAPI verbraucher_api(*cfg, web_reader, file_reader, file_writer, 0);
//			verbraucher_api.starte_router_neu();
//		}

		void heartbeat(const char* data_filename, int beat_count) {
			// Serial.println(printf("Date: %4d-%02d-%02d %02d:%02d:%02d\n", year(time), month(time), day(time), hour(time), minute(time), second(time)));
			Local::Api::VerbraucherAPI verbraucher_api(*cfg, web_reader, file_reader, file_writer, beat_count);
			int now_timestamp = verbraucher_api.timestamp;
			if(!now_timestamp) {
				Serial.println("Der timestamp konnte im System nicht korrekt gelesen werden");
				return;
			}

			Local::Api::WechselrichterAPI wechselrichter_api(*cfg, web_reader);
			wechselrichter_api.daten_holen_und_einsetzen(elektroanlage);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			Local::Api::SmartmeterAPI smartmeter_api(*cfg, web_reader);
			smartmeter_api.daten_holen_und_einsetzen(elektroanlage);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			Local::Api::WettervorhersageAPI wettervorhersage_api(*cfg, web_reader);

			if(!Local::SemipersistentData::stunden_wettervorhersage_letzter_abruf) {
				Local::SemipersistentData::stunden_wettervorhersage_letzter_abruf = now_timestamp;
			}
			if(
				Local::SemipersistentData::stunden_wettervorhersage_letzter_abruf < now_timestamp - 60*45// max alle 45min
				&& minute(now_timestamp) < 20
				&& minute(now_timestamp) >= 10
			) {// Insgesamt also 1x die Stunde ca 10 nach um
				Serial.println("Schreibe Stunden-Wettervorhersage");
				wettervorhersage_api.stundendaten_holen_und_persistieren(file_reader, file_writer);
				Local::SemipersistentData::stunden_wettervorhersage_letzter_abruf = now_timestamp;
				yield();
			}

			if(!Local::SemipersistentData::tages_wettervorhersage_letzter_abruf) {
				Local::SemipersistentData::tages_wettervorhersage_letzter_abruf = now_timestamp;
			}
			if(
				Local::SemipersistentData::tages_wettervorhersage_letzter_abruf < now_timestamp - (3600*4)
			) {// Insgesamt also 1x alle 4 Stunden
				Serial.println("Schreibe Tages-Wettervorhersage");
				wettervorhersage_api.tagesdaten_holen_und_persistieren(file_reader, file_writer);
				Local::SemipersistentData::tages_wettervorhersage_letzter_abruf = now_timestamp;
				yield();
			}
			wettervorhersage_api.persistierte_daten_einsetzen(file_reader, file_writer, wetter, now_timestamp);

			verbraucher_api.daten_holen_und_einsetzen(verbraucher, elektroanlage, wetter);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			verbraucher.wohnraum_temperatur = temperature;
			verbraucher.heizungs_temperatur_differenz = heat_difference;
			verbraucher_api.fuehre_schaltautomat_aus(verbraucher);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			_write_log_data(now_timestamp, verbraucher.heizstabbetrieb_ist_erlaubt);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben

			if(file_writer.open_file_to_overwrite(data_filename)) {
				int anteil_pv1_in_prozent = elektroanlage.gib_anteil_pv1_in_prozent();
				file_writer.write_formated(
					"{\"ueberschuss_in_wh\":%i,\"solarakku_ist_an\":%s,",
					elektroanlage.gib_ueberschuss_in_w(),
					(elektroanlage.solarakku_ist_an ? "true" : "false")
				);
				file_writer.write_formated(
					"\"solarakku_ladestand_in_promille\":%i,",
					elektroanlage.solarakku_ladestand_in_promille
				);
				file_writer.write_formated(
					"\"solaranteil_in_prozent_string1\":%i,",
					anteil_pv1_in_prozent
				);
				file_writer.write_formated(
					"\"solaranteil_in_prozent_string2\":%i,",
					100 - anteil_pv1_in_prozent
				);
				file_writer.write_formated(
					"\"stunden_balkenanzeige_startzeit\":%i,",
					wetter.stundenvorhersage_startzeitpunkt
				);
				yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
				int stundenvorhersage[12];
				for(int i = 0; i < 12; i++) {
					stundenvorhersage[i] = verbraucher.gib_stundenvorhersage_akku_ladestand_als_fibonacci(i);
				}
				file_writer.write_formated(
					"\"stunden_balkenanzeige\":"
				);
				file_writer.write_formated(
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
				file_writer.write_formated(
					"\"tage_balkenanzeige_startzeit\":%i,",
					wetter.tagesvorhersage_startzeitpunkt
				);
				int tagesvorhersage[5];
				for(int i = 0; i < 5; i++) {
					tagesvorhersage[i] = wetter.gib_tagesvorhersage_solarstrahlung_als_fibonacci(i);
				}
				file_writer.write_formated(
					"\"tage_balkenanzeige\":[%i,%i,%i,%i,%i],",
					tagesvorhersage[0],
					tagesvorhersage[1],
					tagesvorhersage[2],
					tagesvorhersage[3],
					tagesvorhersage[4]
				);
				yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
				file_writer.write_formated(
					"\"auto_laden_an\":%s,",
					verbraucher.auto_laden_ist_an() ? "true" : "false"
				);
				file_writer.write_formated(
					"\"auto_relay_an\":%s,",
					verbraucher.auto_relay_ist_an ? "true" : "false"
				);
				file_writer.write_formated(
					"\"auto_lastschutz\":%s,",
					verbraucher.auto_lastschutz ? "true" : "false"
				);
				file_writer.write_formated(
					"\"roller_laden_an\":%s,",
					verbraucher.roller_laden_ist_an() ? "true" : "false"
				);
				file_writer.write_formated(
					"\"roller_relay_an\":%s,",
					verbraucher.roller_relay_ist_an ? "true" : "false"
				);
				file_writer.write_formated(
					"\"roller_lastschutz\":%s,",
					verbraucher.roller_lastschutz ? "true" : "false"
				);
				file_writer.write_formated(
					"\"wasser_ueberladen\":%s,",
					verbraucher.wasser_relay_ist_an ? "true" : "false"
				);
				file_writer.write_formated(
					"\"wasser_lastschutz\":%s,",
					verbraucher.wasser_lastschutz ? "true" : "false"
				);
				file_writer.write_formated(
					"\"heizung_ueberladen\":%s,",
					verbraucher.heizung_relay_ist_an ? "true" : "false"
				);
				file_writer.write_formated(
					"\"heizung_lastschutz\":%s,",
					verbraucher.heizung_lastschutz ? "true" : "false"
				);
				file_writer.write_formated(
					"\"auto_laden\":%s,\"roller_laden\":%s,",
					verbraucher.auto_ladestatus == Local::Model::Verbraucher::Ladestatus::force
						? "\"force\"" : "\"solar\"",
					verbraucher.roller_ladestatus == Local::Model::Verbraucher::Ladestatus::force
						? "\"force\"" : "\"solar\""
				);
				yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
				file_writer.write_formated(
					"\"auto_ladestatus_seit\":%i,",
					verbraucher.auto_ladestatus_seit
				);
				file_writer.write_formated(
					"\"roller_ladestatus_seit\":%i,",
					verbraucher.roller_ladestatus_seit
				);
//				file_writer.write_formated(
//					"\"auto_benoetigte_ladeleistung_in_w\":%i,",
//					verbraucher.auto_benoetigte_ladeleistung_in_w
//				);
//				file_writer.write_formated(
//					"\"roller_benoetigte_ladeleistung_in_w\":%i,",
//					verbraucher.roller_benoetigte_ladeleistung_in_w
//				);
				file_writer.write_formated(
					"\"roller_ladeort_ist_keller\":%s,",
					verbraucher.roller_benoetigte_ladeleistung_in_w == cfg->roller_benoetigte_leistung_gering_in_w
					? "false" : "true"
				);
				file_writer.write_formated(
					"\"ladeverhalten_wintermodus\":%s,",
					verbraucher.ladeverhalten_wintermodus ? "true" : "false"
				);
				file_writer.write_formated(
					"\"solarerzeugung_ist_aktiv\":%s,",
					verbraucher.solarerzeugung_ist_aktiv() ? "true" : "false"
				);
				file_writer.write_formated(
					"\"ersatzstrom_ist_aktiv\":%s,",
					elektroanlage.ersatzstrom_ist_aktiv ? "true" : "false"
				);
				yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
				file_writer.write_formated(
					"\"wohnraum_temperatur\":%.1f,\"wohnraum_luftfeuchtigkeit\":%.1f,",
					temperature,
					humidity
				);
				file_writer.write_formated(
					"\"keller_temperatur\":%.1f,",
					cellar_temperature
				);
				file_writer.write_formated(
					"\"keller_luftfeuchtigkeit\":%.1f,",
					cellar_humidity
				);
				file_writer.write_formated(
					"\"heizung_temperatur_differenz\":%d,",
					heat_difference
				);
				file_writer.write_formated(
					"\"heizstab_erlaubt\":%s,",
					(verbraucher.heizstabbetrieb_ist_erlaubt ? "true" : "false")
				);
				file_writer.write_formated(
					"\"timestamp\":%i}",
					now_timestamp
				);
				file_writer.close_file();
			}
		}
	};
}
