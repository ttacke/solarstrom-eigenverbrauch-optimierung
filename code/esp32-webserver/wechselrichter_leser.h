#pragma once
#include "base_leser.h"

namespace Local {
	class WechselrichterLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	public:
		void daten_holen_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			String content = web_client.get(cfg.wechselrichter_data_url);
			// TODO Das sollte schon so aus dem Netz geholt werden
			int offset = 0;
			int block_size = 64;
			// TODO String sind boese an der Stelle. Geht das besser? als Char aus dem Wechselrichter? Wie geht das?
			// Dann aber gleich das Stream-Lesen blockweise, nicht zerlegen
			String block;
			String old_block = "";
			while(true) {
				block = content.substring(offset, offset + block_size - 1);

				String hit = _finde(
					(char*) "\"P_PV\":([-0-9.]+)[^0-9]",
					old_block + block
				);
				if(hit.length() > 0) {
					elektroanlage.solarerzeugung_in_wh = round(hit.toFloat());
				}
				hit = _finde(
					(char*) "\"P_Akku\":([-0-9.]+)[^0-9]",
					old_block + block
				);
				if(hit.length() > 0) {
					elektroanlage.solarakku_zuschuss_in_wh = round(hit.toFloat());
				}
				hit = _finde(
					(char*) "\"SOC\":([-0-9.]+)[^0-9]",// + funktioniert leider nicht
					old_block + block
				);
				if(hit.length() > 0) {
					elektroanlage.solarakku_ladestand_in_promille = round(hit.toFloat() * 10);
				}
				hit = _finde(
					(char*) "\"P_Grid\":([-0-9.]+)[^0-9]",
					old_block + block
				);
				if(hit.length() > 0) {
					elektroanlage.solarakku_ladestand_in_promille = round(hit.toFloat());
				}
				hit = _finde(
					(char*) "\"P_Load\":([-0-9.]+)[^0-9]",
					old_block + block
				);
				if(hit.length() > 0) {
					elektroanlage.stromverbrauch_in_wh = round(hit.toFloat() * -1);
				}
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
				hit = _finde(
					(char*) "\"BatMode\":([0-9]+)[^0-9]",
					old_block + block
				);
				if(hit.length() > 0) {
					int batt_mode = hit.toInt() * -1;// das ist ne Dezimalzahl, warum auch immer
					if(batt_mode == 1 || batt_mode == 14) {
						elektroanlage.solarakku_ist_an = true;
					} else {
						elektroanlage.solarakku_ist_an = false;
					}
				}

				old_block = block;
				offset += block.length();
				if(offset >= content.length()) {
					break;
				}
			}
		}
	};
}
