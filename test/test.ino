#include "config.h"
#include "wlan.h"
#include "web_reader.h"

Local::Config cfg;
Local::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
Local::WebReader web_reader(wlan.client);

void setup(void) {
	Serial.begin(cfg.log_baud);
	for(int i = 0; i < 5; i++) {
		Serial.print('o');
		delay(500);
	}
	Serial.println("\nStart Test");
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
}

bool _run_test() {
	if(!wlan.is_connected()) {
		Serial.println("Connect WLAN");
		wlan.connect();
		if(!wlan.is_connected()) {
			return false;
		}
	}
	if(!web_reader.send_http_get_request(
		(char*) "192.168.0.33",
		80,
		(char*) "/relay"
	)) {
		Serial.println("Webreader-Read: Failed");
		return false;
	}
	while(web_reader.read_next_block_to_buffer()) {
		Serial.println(web_reader.buffer);
		if(web_reader.find_in_buffer((char*) "\"ison\":")) {
			Serial.println("Webreader-Find: Success");
			return true;
		}
	}
	Serial.println("Webreader-Find: Failed");
	return false;
}

void loop(void) {
	Serial.println("Run test:");
	if(_run_test()) {
		Serial.println("End test: Success");
	} else {
		Serial.println("End test: Error");
	}
	delay(6000000);
}
