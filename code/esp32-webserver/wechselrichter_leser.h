#pragma once
//#include <ArduinoJson.h>
#include "base_leser.h"

namespace Local {
	class WechselrichterLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	public:
		void daten_holen_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			String content = web_client.get(cfg.wechselrichter_data_url);
			elektroanlage.solarerzeugung_in_wh = round(_finde(
				(char*) "\"P_PV\":([-0-9.]+)[^0-9]",
				content
			).toFloat());
			elektroanlage.solarakku_zuschuss_in_wh = round(_finde(
				(char*) "\"P_Akku\":([-0-9.]+)[^0-9]",
				content
			).toFloat());
			elektroanlage.solarakku_ladestand_in_promille = round(_finde(
				(char*) "\"SOC\":([-0-9.]+)[^0-9]",
				content
			).toFloat() * 10);
			elektroanlage.solarakku_ladestand_in_promille = round(_finde(
				(char*) "\"P_Grid\":([-0-9.]+)[^0-9]",
				content
			).toFloat());
			elektroanlage.stromverbrauch_in_wh = round(_finde(
				(char*) "\"P_Load\":([-0-9.]+)[^0-9]",
				content
			).toFloat() * -1);

//			DynamicJsonDocument d = string_to_json(
//				web_client.get(cfg.wechselrichter_data_url)
//			);
//			elektroanlage.solarerzeugung_in_wh = round((float) d["site"]["P_PV"]);
//			elektroanlage.solarakku_zuschuss_in_wh = round((float) d["site"]["P_Akku"]);
//			elektroanlage.solarakku_ladestand_in_promille = round((float) d["inverters"][0]["SOC"] * 10);
//			elektroanlage.netzbezug_in_wh = round((float) d["site"]["P_Grid"]);
//			elektroanlage.stromverbrauch_in_wh = round((float) d["site"]["P_Load"]) * -1;
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
			int batt_mode = _finde(
				(char*) "\"BatMode\":([0-9]+)[^0-9]",
				content
			).toInt() * -1;// das ist ne Dezimalzahl, warum auch immer
//			int batt_mode = (int) d["inverters"][0]["BatMode"];// das ist ne Dezimalzahl, warum auch immer
			if(batt_mode == 1 || batt_mode == 14) {
				elektroanlage.solarakku_ist_an = true;
			} else {
				elektroanlage.solarakku_ist_an = false;
			}
		}
	};
}
