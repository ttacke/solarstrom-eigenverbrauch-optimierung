//*TODO
// Diode an ausgang: leuchtet, wenn heizstab ein
// Zum loggen erst mal Daten lesen und mitschreiben (extern)
// Dann ermitteln, welche Werte ok sind
// dann schaltgrenzen festlegen
// dann messen, und wenn schalten oder x minuten rum, dann shelly-1 aufrufen und schalten
// geht das so herum überhaupt?

// TODO roller-shellies: button-url anpassen. ShellyButton: ebenso.
#include "config.h"
#include "wlan.h"
#include "web_reader.h"
#include "webserver.h"

Local::Config cfg;
Local::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
Local::WebReader web_reader(wlan.client);
Local::Webserver webserver(cfg.webserver_port);
unsigned long runtime;
unsigned long last_runtime;
int sensor_value = 0;

void setup(void) {
	Serial.begin(cfg.log_baud);
	for(int i = 0; i < 5; i++) {
		Serial.print('o');
		delay(500);
	}
	Serial.println("\nStart strom-eigenverbrauch-optimierung Heizungsunterstuetzung");
	wlan.connect();

	webserver.add_http_get_handler("/", []() {
		webserver.server.send(200, "text/plain", (String) "val:" + sensor_value);
	});
	webserver.start();
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());

	// DEBUG
	//pinMode(D4, OUTPUT);
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

void loop(void) {
	runtime = millis();// Will overflow after ~50 days, but this is not a problem
	if(last_runtime < 50 || runtime - last_runtime > 5 * 60000) {// initial || 5min
		Serial.println("heartbeat!");
		while(!_check_network_connection()) {
			wlan.reconnect();
			delay(500);
		}
		last_runtime = runtime;
		return;
	}
	webserver.watch_for_client();
	sensor_value = analogRead(A0);
	// TODO Wenn Heizung an, dann schalten!
	// Serial.println(sensor_value);
	// digitalWrite(D4, HIGH); An/aus
	delay(50);
}
