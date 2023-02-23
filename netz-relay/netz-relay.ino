#include "config.h"
#include "wlan.h"
#include "webserver.h"

Local::Config cfg;
Local::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
Local::Webserver webserver(cfg.webserver_port);
bool relay_is_set = false;
char answer_template[] = "{\"relay_is_set\":\"%s\"}";
char answer[64];

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

		strcpy(answer, answer_template);
		sprintf(answer, relay_is_set ? "true" : "false");
		webserver.server.send(200, "application/json", answer);
	});
	webserver.start();
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
}

void loop(void) {
	webserver.watch_for_client();
	delay(1000);// Ein bisschen ausgebremst ist er immer noch schnell genug
}
