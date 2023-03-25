#pragma once
#include "base_api.h"
#include "model/elektro_anlage.h"

namespace Local {
	class WechselrichterAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		std::uint8_t findings;
		std::uint8_t findings2;

		void _daten_extrahieren_und_einsetzen(Local::Model::ElektroAnlage& elektroanlage) {
			if(web_reader->find_in_buffer((char*) "\"P_PV\":([-0-9.]+)[^0-9]")) {
				elektroanlage.solarerzeugung_in_w = round(atof(web_reader->finding_buffer));
				findings |= 0b0000'0001;
			}
			if(web_reader->find_in_buffer((char*) "\"P_Akku\":([-0-9.]+)[^0-9]")) {
				elektroanlage.solarakku_zuschuss_in_w = round(atof(web_reader->finding_buffer));
				findings |= 0b0000'0010;
			}
			if(web_reader->find_in_buffer((char*) "\"SOC\":([-0-9.]+)[^0-9]")) {
				elektroanlage.solarakku_ladestand_in_promille = round(atof(web_reader->finding_buffer) * 10);
				findings |= 0b0000'0100;
			}
			if(web_reader->find_in_buffer((char*) "\"P_Grid\":([-0-9.nul]+)[^0-9]")) {
				if(strcmp(web_reader->finding_buffer, "null") == 0) {
					elektroanlage.ersatzstrom_ist_aktiv = true;
					elektroanlage.netzbezug_in_w = 0;
				} else {
					elektroanlage.ersatzstrom_ist_aktiv = false;
					elektroanlage.netzbezug_in_w = round(atof(web_reader->finding_buffer));
				}
				findings |= 0b0000'1000;
			}
			if(web_reader->find_in_buffer((char*) "\"P_Load\":([-0-9.]+)[^0-9]")) {
				elektroanlage.stromverbrauch_in_w = round(atof(web_reader->finding_buffer) * -1);
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
			if(web_reader->find_in_buffer((char*) "\"BatMode\":([0-9]+)[^0-9]")) {// das ist ne Dezimalzahl, warum auch immer
				int batt_mode = atoi(web_reader->finding_buffer);
				if(batt_mode == 1 || batt_mode == 14) {
					elektroanlage.solarakku_ist_an = true;
				} else {
					elektroanlage.solarakku_ist_an = false;
				}
				findings |= 0b0010'0000;
			}
		}

		void _detaildaten_einsetzen(Local::Model::ElektroAnlage& elektroanlage) {
			if(web_reader->find_in_buffer((char*) "\"PV_POWERACTIVE_MEAN_01_F32\" *: *([0-9.]+)[^0-9]")) {
				elektroanlage.leistungsanteil_pv1 = round(atof(web_reader->finding_buffer) * 1000);
				findings |= 0b0100'0000;
			}
			if(web_reader->find_in_buffer((char*) "\"PV_POWERACTIVE_MEAN_02_F32\" *: *([0-9.]+)[^0-9]")) {
				elektroanlage.leistungsanteil_pv2 = round(atof(web_reader->finding_buffer) * 1000);
				findings |= 0b1000'0000;
			}
			if(web_reader->find_in_buffer((char*) "\"ACBRIDGE_CURRENT_ACTIVE_MEAN_01_F32\" *: *([-0-9.]+)[^0-9]")) {
				elektroanlage.l1_solarstrom_ma = round(atof(web_reader->finding_buffer) * 1000);
				findings2 |= 0b0000'0001;
			}
		}

	public:
		void daten_holen_und_einsetzen(Local::Model::ElektroAnlage& elektroanlage) {
			web_reader->send_http_get_request(
				cfg->wechselrichter_host,
				cfg->wechselrichter_port,
				"/status/powerflow"
			);
			findings = 0b0000'0000;
			findings2 = 0b0000'0000;
			while(web_reader->read_next_block_to_buffer()) {
				_daten_extrahieren_und_einsetzen(elektroanlage);
			}
			web_reader->send_http_get_request(
				cfg->wechselrichter_host,
				cfg->wechselrichter_port,
				"/components/cache/readable"
			);
			while(web_reader->read_next_block_to_buffer()) {
				_detaildaten_einsetzen(elektroanlage);
			}
			if(!(findings & 0b1111'1111) && !(findings2 & 0b0000'0001)) {
				elektroanlage.solarerzeugung_in_w = 0;
				elektroanlage.solarakku_zuschuss_in_w = 0;
				elektroanlage.solarakku_ladestand_in_promille = 0;
				elektroanlage.netzbezug_in_w = 0;
				elektroanlage.stromverbrauch_in_w = 0;
				elektroanlage.solarakku_ist_an = false;
				elektroanlage.leistungsanteil_pv1 = 0;
				elektroanlage.leistungsanteil_pv2 = 0;
				elektroanlage.l1_solarstrom_ma = 0;
			}
		}
	};
}
