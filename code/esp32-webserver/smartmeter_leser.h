#pragma once
#include <ArduinoJson.h>
#include "base_leser.h"

namespace Local {
	class SmartmeterLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	public:
		void daten_holen_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			DynamicJsonDocument d = string_to_json(
				web_client.get(cfg.smartmeter_data_url)
			);
			JsonObject data = d["Body"]["Data"][cfg.smartmeter_id]["channels"];
			elektroanlage.l1_strom_ma = round((float) data["SMARTMETER_CURRENT_01_F64"] * 1000);
			elektroanlage.l2_strom_ma = round((float) data["SMARTMETER_CURRENT_02_F64"] * 1000);
			elektroanlage.l3_strom_ma = round((float) data["SMARTMETER_CURRENT_03_F64"] * 1000);
		}
	};
}
