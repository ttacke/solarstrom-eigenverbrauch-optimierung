#include "config.h"
#include "service/wlan.h"
#include "service/web_reader.h"
#include "web_presenter.h"

Local::Config cfg;
Local::Service::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
Local::Service::WebReader web_reader(wlan.client);
Local::WebPresenter web_presenter(cfg, wlan);
unsigned long runtime;
unsigned long last_runtime;
const char* daten_filename = "daten.json";
int beat_count = 0;
//int network_error_count = 0;

void setup(void) {
	runtime = 0;
	last_runtime = 0;
	Serial.begin(cfg.log_baud);
	for(int i = 0; i < 5; i++) {
		Serial.print('o');
		delay(500);
	}
	Serial.println("Start strom-eigenverbrauch-optimierung Zentrale");
	wlan.connect();

	web_presenter.webserver.add_http_get_handler("/", []() {
		web_presenter.zeige_ui();
	});
	web_presenter.webserver.add_http_get_handler("/change", []() {
		web_presenter.aendere();
	});
	web_presenter.webserver.add_http_get_handler("/daten.json", []() {
		web_presenter.download_file(daten_filename);
	});
	web_presenter.webserver.add_http_get_handler("/download_file", []() {
		web_presenter.download_file((const char*) web_presenter.webserver.server.arg("name").c_str());
	});
	web_presenter.webserver.add_http_post_fileupload_handler("/upload_file", []() {
		web_presenter.upload_file();
	});
	web_presenter.webserver.add_http_get_handler("/set_temperature_and_humidity", []() {// &hum=45&temp=22.38
		web_presenter.set_temperature_and_humidity(
		    atof(web_presenter.webserver.server.arg("temp").c_str()),
		    atof(web_presenter.webserver.server.arg("hum").c_str())
		);
	});
	web_presenter.webserver.add_http_get_handler("/set_cellar_temperature_and_humidity", []() {// &hum=45&temp=22.38
		web_presenter.set_cellar_temperature_and_humidity(
		    atof(web_presenter.webserver.server.arg("temp").c_str()),
		    atof(web_presenter.webserver.server.arg("hum").c_str())
		);
	});
	web_presenter.webserver.add_http_get_handler("/set_heat_difference", []() {// &val=537
		web_presenter.set_heat_difference(
		    atof(web_presenter.webserver.server.arg("val").c_str())
		);
	});
	web_presenter.webserver.start();
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
}

void(* resetFunc) (void) = 0;// Reset the board via software

bool _check_network_connection() {
	if(!wlan.is_connected()) {
		return false;
	}
	if(!web_reader.send_http_get_request(
		cfg.network_connection_check_host,
		80,
		cfg.network_connection_check_path
	)) {
		return false;
	}
//	Die Test-URL hat keinen ContentLengthAngabe und schlaegt beim Lesen deshalb fehl
//	while(web_reader.read_next_block_to_buffer()) {
//		Serial.println(web_reader.buffer);
//		if(web_reader.find_in_buffer(strdup(cfg.network_connection_check_content))) {
//			return true;
//		}
//	}
//	return false;
	return true;
}
bool _check_internet_connection() {
	if(!wlan.is_connected()) {
		return false;
	}
	// TODO: internes Gateway irgendwie erreichbar? Kann das intern gemacht werden?
	if(!web_reader.send_http_get_request(
		cfg.internet_connection_check_host,
		80,
		cfg.internet_connection_check_path
	)) {
		return false;
	}
	return true;
}

void loop(void) {
	runtime = millis();// Will overflow after ~50 days, but this is not a problem
	if(runtime == 0) {// heartbeat läuft auch mal endlos duch. Kann eigentlich nur diesen Grund haben
		runtime = last_runtime + 50;
	}
	if(last_runtime == 0 || runtime - last_runtime > 60000) {// initial || 1min
		Serial.println("heartbeat!");
		beat_count++;
		if(beat_count % 5 == 0) {// Nur alle 5 Beats pruefen (derzeit 5min)
			while(!_check_network_connection()) {
				wlan.reconnect();
				delay(500);
			}
			if(beat_count >= 1000000) {
				beat_count = 0;
			}
//			if(!_check_internet_connection()) {
//				network_error_count++;
//				if(network_error_count >= 3) {
//					web_presenter.restart_router();
//					delay(10000);
//					return;
//				}
//			}
		}
		web_presenter.heartbeat(daten_filename, beat_count);
		last_runtime = runtime;
		return;
	}
	web_presenter.webserver.watch_for_client();
	delay(50);
}
