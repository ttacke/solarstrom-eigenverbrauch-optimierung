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
	for(int i = 0; i < 5; i++) {
		Serial.print('o');
		delay(500);
	}
	Serial.println("Start strom-eigenverbrauch-optimierung Zentrale");
	wlan.connect();

	web_presenter.webserver.add_http_get_handler("/master/", []() {
		web_presenter.zeige_ui();
	});
	web_presenter.webserver.add_http_get_handler("/master/daten.json", []() {
		web_presenter.zeige_daten(true);
	});
	web_presenter.webserver.add_http_get_handler("/master/change", []() {
		web_presenter.aendere();
	});

	web_presenter.webserver.add_http_get_handler("/", []() {
		web_presenter.zeige_ui();
	});
	web_presenter.webserver.add_http_get_handler("/daten.json", []() {
		web_presenter.zeige_daten(false);
	});
	web_presenter.webserver.add_http_get_handler("/change", []() {
		web_presenter.webserver.server.send(403, "text/plain", "For master only!");
	});

	web_presenter.webserver.add_http_get_handler("/download_file", []() {
		web_presenter.download_file();
	});
	web_presenter.webserver.add_http_post_fileupload_handler("/upload_file", []() {
		web_presenter.upload_file();
	});
	web_presenter.webserver.start();
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
}

void loop(void) {
	web_presenter.webserver.watch_for_client();
	delay(1000);// Ein bisschen ausgebremst ist er immer noch schnell genug
}
