#include "config.h"
#include "wlan.h"
#include "web_presenter.h"

Local::Config cfg;
Local::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
Local::WebPresenter web_presenter(cfg, wlan);

void setup(void) {
	Serial.begin(cfg.log_baud);
	Serial.println("\nSetup ESP");
	wlan.connect();
	web_presenter.webserver.add_page("/", []() {
		web_presenter.zeige_hauptseite();
		Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
	});
	web_presenter.webserver.start();
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
}

void loop(void) {
	web_presenter.webserver.watch_for_client();
}
