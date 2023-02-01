// TODO bis auf ein Minimum ist alles deaktiviert, um das Speicherproblem zu beheben
/*
	- WLAN blockweise lesen
	- alles auf bekannte speichergroessen begrenzen
	- char[] nutzen, statt String
	- immer pointer herumreichen. Im Zweiffen den Rückgabewert IN die Funktion geben. malloc+free ist nur ein Notnagel
	- Objektreferenzen immer nutzen, nie kopien
	- Wenn geht, für alles dauerhafte Variablen nutzen, wo der Speicher fixiert ist und dieser dann wiederverwendet wird
	-- Grund: Speicherfragmentierung vermeiden
*/
#include "config.h"
#include "wlan.h"
#include "web_client.h"
//#include "web_presenter.h"

//#include "elektro_anlage.h"
//#include "wechselrichter_leser.h"


Local::Config cfg;
Local::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
//Local::WebPresenter web_presenter(cfg, wlan);

void setup(void) {
	Serial.begin(cfg.log_baud);
	delay(10);
	Serial.println();
	Serial.println();
	Serial.println("Setup ESP");
	wlan.connect();
//	web_presenter.webserver.add_page("/", []() {
//		web_presenter.zeige_hauptseite();
//		Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
//	});
//	web_presenter.webserver.start();
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
}

void loop(void) {
//	web_presenter.webserver.watch_for_client();
// TODO das macht immer nur 2 Runden. Wieso?
	Serial.println("LOOP");
	Local::WebClient web_client(wlan.client);
	web_client.send_http_get_request("192.168.0.14", 80, "/status/powerflow");
	while(web_client.read_next_block_to_buffer()) {
		Serial.println(web_client.buffer);
	}
//	Local::WechselrichterLeser wechselrichter_leser(cfg, web_client);
//	Local::ElektroAnlage elektroanlage;
//	wechselrichter_leser.daten_holen_und_einsetzen(elektroanlage);
//	Serial.println(elektroanlage.gib_log_zeile());
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
	delay(5000);
}
