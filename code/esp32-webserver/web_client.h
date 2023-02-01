#pragma once
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

namespace Local {
	class WebClient {
	protected:
		WiFiClient wlan_client;

	public:
		WebClient(WiFiClient& wlan_client_param) {
			wlan_client = wlan_client_param;
		}
		// TODO DEPRECATED, soll in die Leser verschoben werden

		// TODO hier weiter: Blockweise lesen!
		// https://stackoverflow.com/questions/34078497/esp8266-wificlient-simple-http-get
		void get_http_content() {//char* host, char* url) {
			const int httpPort = 80;
			const char* host = "192.168.0.14";
			if(!wlan_client.connect(host, httpPort)) {
				Serial.println("connection failed");
				return;// TODO wie damit umgehen?
			}
			String url = "/status/powerflow";
			// printBytes? Das sollte einfacher sein, ohne String-Foo
			wlan_client.print(String("GET ") + url + " HTTP/1.1\r\n" +
				"Host: " + host + "\r\n" +
				"Connection: close\r\n\r\n");
			int i = 0;
			while(!wlan_client.available()) {// Warten, bis Verbindung steht
				i++;
				if(i > 1000) {// 10s timeout
					return;// TODO wie damit umgehen
				}
				delay(10);
			}
			// TODO das hier extern nutzbar mache (read next oder so)
			// dazu ist der WebClient dann wieder nuetzlich
			// wlan_client.clear() oder .close() oder .disconnect()???
			while(wlan_client.available()){
				char buf[64];
				// https://arduinogetstarted.com/reference/serial-readbytes
				int rlen = wlan_client.readBytes(buf, 63);
				buf[rlen] = '\0';
				Serial.println(buf);
				Serial.println("---");
			}
		}

		String get(const char* url) {
//TODO immer charArray nutzen
//		String a = "ABCDE";
//	char b[a.length() + 1];
//	a.toCharArray(b, sizeof(b));
//	Serial.println(strlen(b));
//	Serial.println(b);

// Hier das Blockweise abrufen einbauen! Sonst wird das zu groß

			HTTPClient http;
			http.begin(wlan_client, url);
			http.addHeader("Content-Type", "application/json");
			http.setTimeout(15000);// ms, vermutlich
			int httpCode = http.GET();
			if (httpCode > 0) {
				if (httpCode == HTTP_CODE_OK) {
				  const String payload = http.getString();
				  http.end();
				  return payload;
			  } else {
				Serial.printf("[HTTP] GET failed, code: %d\n", httpCode);
			  }
			} else {
			  Serial.printf("[HTTP] GET failed, error: ");
			  Serial.printf(http.errorToString(httpCode).c_str());
			}
			http.end();
			return "";
		}
	};
}
