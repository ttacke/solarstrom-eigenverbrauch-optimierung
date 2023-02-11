#pragma once
#include "base_leser.h"
#include "elektro_anlage.h"

namespace Local {
	class WechselrichterLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	protected:
		std::uint8_t findings;

		void _daten_extrahieren_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			if(web_client->find_in_content((char*) "\"P_PV\":([-0-9.]+)[^0-9]")) {
				elektroanlage.solarerzeugung_in_wh = round(atof(web_client->finding_buffer));
				findings |= 0b0000'0001;
			}
			if(web_client->find_in_content((char*) "\"P_Akku\":([-0-9.]+)[^0-9]")) {
				elektroanlage.solarakku_zuschuss_in_wh = round(atof(web_client->finding_buffer));
				findings |= 0b0000'0010;
			}
			if(web_client->find_in_content((char*) "\"SOC\":([-0-9.]+)[^0-9]")) {
				elektroanlage.solarakku_ladestand_in_promille = round(atof(web_client->finding_buffer) * 10);
				findings |= 0b0000'0100;
			}
			if(web_client->find_in_content((char*) "\"P_Grid\":([-0-9.]+)[^0-9]")) {
				elektroanlage.netzbezug_in_wh = round(atof(web_client->finding_buffer));
				findings |= 0b0000'1000;
			}
			if(web_client->find_in_content((char*) "\"P_Load\":([-0-9.]+)[^0-9]")) {
				elektroanlage.stromverbrauch_in_wh = round(atof(web_client->finding_buffer) * -1);
				findings |= 0b0001'0000;
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
			if(web_client->find_in_content((char*) "\"BatMode\":([0-9]+)[^0-9]")) {// das ist ne Dezimalzahl, warum auch immer
				int batt_mode = atoi(web_client->finding_buffer);
				if(batt_mode == 1 || batt_mode == 14) {
					elektroanlage.solarakku_ist_an = true;
				} else {
					elektroanlage.solarakku_ist_an = false;
				}
				findings |= 0b0010'0000;
			}
		}

		void _verteilungsdaten_extrahieren_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			if(web_client->find_in_content((char*) "\"PV_POWERACTIVE_MEAN_01_F32\" *: *([0-9.]+)[^0-9]")) {
				elektroanlage.leistungsanteil_pv1 = round(atof(web_client->finding_buffer) * 1000);
				findings |= 0b0100'0000;
			}
			if(web_client->find_in_content((char*) "\"PV_POWERACTIVE_MEAN_02_F32\" *: *([0-9.]+)[^0-9]")) {
				elektroanlage.leistungsanteil_pv2 = round(atof(web_client->finding_buffer) * 1000);
				findings |= 0b1000'0000;
			}
		}

	public:
		void daten_holen_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			web_client->send_http_get_request(
				cfg->wechselrichter_host,
				cfg->wechselrichter_port,
				"/status/powerflow"
			);
			findings = 0b0000'0000;
			while(web_client->read_next_block_to_buffer()) {
				_daten_extrahieren_und_einsetzen(elektroanlage);
			}
			web_client->send_http_get_request(
				cfg->wechselrichter_host,
				cfg->wechselrichter_port,
				"/components/cache/readable"
			);
			while(web_client->read_next_block_to_buffer()) {
				_verteilungsdaten_extrahieren_und_einsetzen(elektroanlage);
			}
			if(!(findings & 0b1111'1111)) {
				elektroanlage.solarerzeugung_in_wh = 0;
				elektroanlage.solarakku_zuschuss_in_wh = 0;
				elektroanlage.solarakku_ladestand_in_promille = 0;
				elektroanlage.netzbezug_in_wh = 0;
				elektroanlage.stromverbrauch_in_wh = 0;
				elektroanlage.solarakku_ist_an = false;
				elektroanlage.leistungsanteil_pv1 = 0;
				elektroanlage.leistungsanteil_pv2 = 0;
			}
		}
	};
}
