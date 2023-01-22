#pragma once
#include "web_client.h"
#include "config.h"
#include <ArduinoJson.h>

class WechselrichterLeser {
protected:
	WebClient web_client;
	Config cfg;
	DynamicJsonDocument string_to_json(String content) {
	  DynamicJsonDocument doc(content.length() * 2);
	  DeserializationError error = deserializeJson(doc, content);
	  if (error) {
		Serial.print(F("deserializeJson() failed with code "));
		Serial.println(error.c_str());
		return doc;
	  }
	  return doc;
	}
public:
	WechselrichterLeser(Config cfg, WebClient web_client): cfg(cfg), web_client(web_client) {
	}
	void daten_holen_und_einsetzen(ElektroAnlage& elektroanlage) {
		DynamicJsonDocument d = string_to_json(
			web_client.get(cfg.wechselrichter_data_url)
		);
		elektroanlage.solar_wh = round((float) d["site"]["P_PV"]);
		elektroanlage.solarakku_wh = round((float) d["site"]["P_Akku"]);
		elektroanlage.solarakku_ladestand_prozent = round((float) d["site"]["P_Akku"]);
		elektroanlage.netz_wh = round((float) d["site"]["P_Grid"]);
		elektroanlage.verbraucher_wh = round((float) d["site"]["P_Load"]);
		/*
			http://192.168.0.106/main-es2015.57cf3de98e3c435ccd68.js
			BatMode
			case 0: "disabled"
			-- 1: aktiv
			case 2: "serviceMode"
			case 3: "chargeBoost"
			case 4: "nearlyDepleted"
			case 5: "suspended"
			case 6: "calibration"
			case 8: "depletedRecovery"
			case 12: "startup"
			case 13: "stoppedTemperature"
			case 14: "maxSocReached"
		*/
		if(((String) d["inverters"][0]["BatMode"]).compareTo("1")) {
			elektroanlage.solarakku_ist_an = true;
		} else {
			elektroanlage.solarakku_ist_an = false;
		}
	}
};
