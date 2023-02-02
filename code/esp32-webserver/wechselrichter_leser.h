#pragma once
#include "base_leser.h"
#include "elektro_anlage.h"

namespace Local {
	class WechselrichterLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	protected:
		// TODO die Daten werden nicht immer korrekt gelesen. Wieso? Sind die zu lang für die Buffer? Mehr als 32 Zeichen?
		void _daten_extrahieren_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			if(web_client.find_in_content((char*) "\"P_PV\":([-0-9.]+)[^0-9]")) {
				elektroanlage.solarerzeugung_in_wh = round(atof(web_client.finding_buffer));
			}
			if(web_client.find_in_content((char*) "\"P_Akku\":([-0-9.]+)[^0-9]")) {
				elektroanlage.solarakku_zuschuss_in_wh = round(atof(web_client.finding_buffer));
			}
			if(web_client.find_in_content((char*) "\"SOC\":([-0-9.]+)[^0-9]")) {
				elektroanlage.solarakku_ladestand_in_promille = round(atof(web_client.finding_buffer) * 10);
			}
			if(web_client.find_in_content((char*) "\"P_Grid\":([-0-9.]+)[^0-9]")) {
				elektroanlage.netzbezug_in_wh = round(atof(web_client.finding_buffer));
			}
			if(web_client.find_in_content((char*) "\"P_Load\":([-0-9.]+)[^0-9]")) {
				elektroanlage.stromverbrauch_in_wh = round(atof(web_client.finding_buffer) * -1);
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
			// TODO das trifft nicht!
			if(web_client.find_in_content((char*) "\"BatMode\":([0-9]+)[^0-9]")) {
				int batt_mode = atoi(web_client.finding_buffer);// das ist ne Dezimalzahl, warum auch immer
				Serial.println("Vatt:");
				Serial.println(batt_mode);
				if(batt_mode == 1 || batt_mode == 14) {
					elektroanlage.solarakku_ist_an = true;
				} else {
					elektroanlage.solarakku_ist_an = false;
				}
			}
		}

	public:
		void daten_holen_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			web_client.send_http_get_request(
				cfg.wechselrichter_host,
				cfg.wechselrichter_port,
				cfg.wechselrichter_data_request_uri
			);
			while(web_client.read_next_block_to_buffer()) {
				_daten_extrahieren_und_einsetzen(elektroanlage);
			}
		}
	};
}
