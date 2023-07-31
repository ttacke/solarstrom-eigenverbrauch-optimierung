#include "config.h"
#include "wlan.h"
#include "web_reader.h"
#include "webserver.h"

Local::Config cfg;
Local::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
Local::WebReader web_reader(wlan.client);
Local::Webserver webserver(cfg.webserver_port);
bool relay_is_set = false;
unsigned long runtime;
unsigned long last_runtime;

void setup(void) {
	Serial.begin(cfg.log_baud);
	for(int i = 0; i < 5; i++) {
		Serial.print('o');
		delay(500);
	}
	Serial.println("\nStart strom-eigenverbrauch-optimierung Relay");
	pinMode(D2, OUTPUT);
	wlan.connect();

	webserver.add_http_get_handler("/relay", []() {
		const char* set_param = webserver.server.arg("set_relay").c_str();
		if(strlen(set_param) > 0) {
			if(strcmp(set_param, "true") == 0) {
				relay_is_set = true;
			} else {
				relay_is_set = false;
			}
		}

		digitalWrite(D2, relay_is_set ? HIGH : LOW);

		webserver.server.send(200, "application/json", relay_is_set ? "{\"ison\":true}" : "{\"ison\":false}");
	});
	webserver.start();
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
}

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
	if(last_runtime == 0 || runtime - last_runtime > 5 * 60000) {// initial || 5min
		Serial.println("heartbeat!");
		while(!_check_network_connection()) {
			wlan.reconnect();
			delay(500);
		}
		last_runtime = runtime;
		return;
	}
	webserver.watch_for_client();
	delay(50);
}
