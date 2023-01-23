#pragma once
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>

namespace Local {
	class WebClient {
	protected:
		WiFiClient wlan_client;
	public:
		WebClient(WiFiClient wlan_client_param) {
			wlan_client = wlan_client_param;
		}
		// DEPRECATED, soll in die Leser
		String get(char const* url) {
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
			return "{\"msg\":\"internal error\"}";
		}
	};
}
