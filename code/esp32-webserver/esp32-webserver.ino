#include "config.h"
#include "wlan.h"
#include "webserver.h"
#include "web_presenter.h"

Local::Config cfg;
Local::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
Local::Webserver webserver(80);
Local::WebPresenter web_presenter(cfg, wlan);

void setup(void) {
	Serial.begin(cfg.log_baud);
	Serial.println("\nSetup ESP");
	wlan.connect();
	webserver.add_page("/", []() {
		web_presenter.zeige_hauptseite(webserver);
		Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
	});
	webserver.start();
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
}

void loop(void) {
	webserver.watch_for_client();
}
