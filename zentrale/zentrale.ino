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
	web_presenter.webserver.add_http_get_handler("/debug-erzeuge-daten.json", []() {
		web_presenter.heartbeat(daten_filename);
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
	while(web_reader.read_next_block_to_buffer()) {
		if(web_reader.find_in_buffer(strdup(cfg.network_connection_check_content))) {
			return true;
		}
	}
	return false;
}

void loop(void) {
	runtime = millis();// Will overflow after ~50 days, but this is not a problem
	if(last_runtime == 0 || runtime - last_runtime > 60000) {// initial || 1min
		while(!_check_network_connection()) {
			wlan.reconnect();
			delay(500);
		}
		Serial.println("heartbeat!");
		web_presenter.heartbeat(daten_filename);
		last_runtime = runtime;
		return;
	}
	web_presenter.webserver.watch_for_client();
	delay(50);
}
