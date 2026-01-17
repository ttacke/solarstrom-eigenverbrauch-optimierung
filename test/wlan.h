#pragma once
#include <WiFiClient.h>
#include <ESP8266WiFi.h>

namespace Local {
	class Wlan {
	public:
		WiFiClient client;
		const char* ssid;
		const char* pwd;

		Wlan(const char* ssid, const char* pwd): ssid(ssid), pwd(pwd) {
		}

		void connect() {
			Serial.println("Try to connect to WLAN " + (String) ssid);
			WiFi.mode(WIFI_STA);
			WiFi.begin(ssid, pwd);
			while (WiFi.status() != WL_CONNECTED) {
				delay(500);
				Serial.print(".");
			}
			Serial.println("\nWLAN Connected, IP address: " + WiFi.localIP().toString() + "\n");
			WiFi.setAutoReconnect(true);
			WiFi.persistent(true);
		}

		bool is_connected() {
			return WiFi.isConnected();
		}

		void reconnect() {
			WiFi.reconnect();
			while (WiFi.status() != WL_CONNECTED) {
			  delay(500);
			  Serial.print(".");
			}
			Serial.println("\nWLAN ReConnected, IP address: " + WiFi.localIP().toString() + "\n");
			WiFi.setAutoReconnect(true);
			WiFi.persistent(true);
		}
	};
}
