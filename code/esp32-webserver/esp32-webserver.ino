// TODO bis auf ein Minimum ist alles deaktiviert, um das Speicherproblem zu beheben
/* TODO Als naechstes:
- HTML Datei von SD-Karte ausliefern
- Daten als JSON ausgeben
- Stromdaten auch in JSON einfuegen
- Wetterdaten in JSON einfuegen
- Wetterdaten in UI darstellen
*/
#include "config.h"
#include "wlan.h"
#include "web_client.h"
#include "web_presenter.h"
#include "elektro_anlage.h"
#include "wechselrichter_leser.h"

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
//	Local::WechselrichterLeser wechselrichter_leser(cfg, web_client);
//	Local::ElektroAnlage elektroanlage;
//	wechselrichter_leser.daten_holen_und_einsetzen(elektroanlage);
//	Serial.println(elektroanlage.gib_log_zeile());
//	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
//	delay(5000);
}
