#pragma once
#include "web_client.h"
#include "config.h"
#include <ArduinoJson.h>

namespace Local {
	class BaseLeser {

	protected:
		Local::WebClient web_client;

		Local::Config cfg;

		DynamicJsonDocument string_to_json(String content) {
		  DynamicJsonDocument doc(content.length() * 2);
		  Serial.println(content.length());
		  DeserializationError error = deserializeJson(doc, content);
		  if (error) {
			Serial.print(F("deserializeJson() failed with code "));
			Serial.println(error.c_str());
			return doc;
		  }
		  return doc;
		}
	public:
		explicit BaseLeser(Local::Config cfg, Local::WebClient web_client): cfg(cfg), web_client(web_client) {
		}

		virtual void daten_holen_und_einsetzen(Local::ElektroAnlage& elektroanlage) = 0;
	};
}
