#pragma once
#include <ArduinoJson.h>
#include "base_leser.h"

namespace Local {
	class SmartmeterLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	public:
		void daten_holen_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			Serial.println("Gogogo");
			DynamicJsonDocument d = string_to_json(
				web_client.get(cfg.smartmeter_data_url)
			);
			Serial.println("Gogogo2");
			JsonObject data = d["Body"]["Data"][cfg.smartmeter_id]["channels"];
			Serial.println("Gogogo3");
			elektroanlage.l1_strom_ma = round((float) data["SMARTMETER_CURRENT_01_F64"] * 1000);
			float i2 = (float) data["SMARTMETER_CURRENT_02_F64"];
			float i3 = (float) data["SMARTMETER_CURRENT_03_F64"];
			Serial.println("Gogogo4");
			Serial.println(elektroanlage.l1_strom_ma);
			//  return _hole_maximalen_strom_und_phase_aus_json(b);
			// elektroanlage.solar_wh = round((float) d["site"]["P_PV"]);
		}
	};
}
