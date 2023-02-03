/* TODO noch offen:
- HTML Datei von SD-Karte ausliefern
- Wetterdaten in UI darstellen
*/
#include "config.h"
#include "wlan.h"
#include "web_client.h"
#include "web_presenter.h"

Local::Config cfg;
Local::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
Local::WebClient web_client(wlan.client);
Local::WebPresenter web_presenter(cfg, wlan);

void setup(void) {
	Serial.begin(cfg.log_baud);
	delay(10);
	Serial.println();
	Serial.println();
	Serial.println("Setup ESP");
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
	delay(1000);// Ein bisschen ausgebremst ist er immer noch schnell genug
}
