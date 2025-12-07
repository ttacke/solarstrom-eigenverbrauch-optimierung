#pragma once
#include "base_api.h"
#include "../model/elektro_anlage.h"

namespace Local::Api {
	class WechselrichterAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		std::uint8_t findings;
		std::uint8_t findings2;

		void _daten_extrahieren_und_einsetzen(Local::Model::ElektroAnlage& elektroanlage) {
			if(web_reader->find_in_buffer((char*) "\"P_PV\" *: *([-0-9.]+)[^0-9]")) {
				elektroanlage.solarerzeugung_in_w = round(atof(web_reader->finding_buffer));
				findings |= 0b0000'0001;
			}
			if(web_reader->find_in_buffer((char*) "\"P_Akku\" *: *([-0-9.]+)[^0-9]")) {
				elektroanlage.solarakku_zuschuss_in_w = round(atof(web_reader->finding_buffer));
				findings |= 0b0000'0010;
			}
			if(web_reader->find_in_buffer((char*) "\"SOC\" *: *([-0-9.]+)[^0-9]")) {
				elektroanlage.solarakku_ladestand_in_promille = round(atof(web_reader->finding_buffer) * 10);
				findings |= 0b0000'0100;
			}
			if(web_reader->find_in_buffer((char*) "\"E_Total\" *: *([0-9]+)[^0-9]")) {
				elektroanlage.gesamt_energiemenge_in_wh = round(atof(web_reader->finding_buffer) * 1);
			}
			if(web_reader->find_in_buffer((char*) "\"P_Grid\" *: *([-0-9.nul]+)[^0-9]")) {
				if(strcmp(web_reader->finding_buffer, "null") == 0) {
					elektroanlage.ersatzstrom_ist_aktiv = true;
					elektroanlage.netzbezug_in_w = 0;
				} else {
					elektroanlage.ersatzstrom_ist_aktiv = false;
					elektroanlage.netzbezug_in_w = round(atof(web_reader->finding_buffer));
				}
				findings |= 0b0000'1000;
			}
			if(web_reader->find_in_buffer((char*) "\"P_Load\" *: *([-0-9.]+)[^0-9]")) {
				elektroanlage.stromverbrauch_in_w = round(atof(web_reader->finding_buffer) * -1);
				findings |= 0b0001'0000;
			}
			if(web_reader->find_in_buffer((char*) "\"Battery_Mode\" : \"([^\"]+)\"")) {
				if(
					strcmp(web_reader->finding_buffer, "battery full") == 0
					|| strcmp(web_reader->finding_buffer, "normal") == 0
					|| strcmp(web_reader->finding_buffer, "charge boost") == 0
				) {
					elektroanlage.solarakku_ist_an = true;
				} else {
					elektroanlage.solarakku_ist_an = false;
				}
				findings |= 0b0010'0000;
			}
		}

// TODO Aufteilung Süd/Nord reparieren
// https://api.open-meteo.com/v1/forecast?latitude=50.00&longitude=7.95&daily=sunrise,sunset,shortwave_radiation_sum&hourly=global_tilted_irradiance_instant&timezone=Europe%2FBerlin&tilt=35&azimuth=135&timeformat=unixtime&forecast_hours=12

// TODO Vorhersage erneuern
// curl http://192.168.0.14/solar_api/v1/GetInverterInfo.cgi
// -> Device: 1
// curl "http://192.168.0.14/solar_api/v1/GetActiveDeviceInfo.cgi?DeviceClass=System"
// curl "http://192.168.0.14/solar_api/v1/GetMeterRealtimeData.cgi?Scope=System"
/*
	-> am "Meter" gemessene Ströhme
	"Current_AC_Phase_1" : -1.236,
	"Current_AC_Phase_2" : -0.94499999999999995,
	"Current_AC_Phase_3" : -1.361,



*/
// curl "http://192.168.0.14/solar_api/v1/GetMeterRealtimeData.cgi?Scope=Device&DeviceId=1"
// Das gleiche wie "System"
// curl http://192.168.0.14/solar_api/v1/GetStorageRealtimeData.cgi
// curl http://192.168.0.14/solar_api/v1/GetOhmPilotRealtimeData.cgi
// curl http://192.168.0.14/solar_api/v1/GetPowerFlowRealtimeData.fcgi
// curl http://192.168.0.14/solar_api/GetInverterRealtimeData.cgi
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
			if(web_reader->find_in_buffer((char*) "\"ACBRIDGE_CURRENT_ACTIVE_MEAN_02_F32\" *: *([-0-9.]+)[^0-9]")) {
				elektroanlage.l2_solarstrom_ma = round(atof(web_reader->finding_buffer) * 1000);
				findings2 |= 0b0000'0010;
			}
			if(web_reader->find_in_buffer((char*) "\"ACBRIDGE_CURRENT_ACTIVE_MEAN_03_F32\" *: *([-0-9.]+)[^0-9]")) {
				elektroanlage.l3_solarstrom_ma = round(atof(web_reader->finding_buffer) * 1000);
				findings2 |= 0b0000'0100;
			}
		}

	public:
		void daten_holen_und_einsetzen(Local::Model::ElektroAnlage& elektroanlage) {
			web_reader->send_http_get_request(
				cfg->wechselrichter_host,
				cfg->wechselrichter_port,
				"/solar_api/v1/GetPowerFlowRealtimeData.fcgi"
			);
			findings = 0b0000'0000;
			findings2 = 0b0000'0000;
			while(web_reader->read_next_block_to_buffer()) {
				_daten_extrahieren_und_einsetzen(elektroanlage);
			}
			// TODO API nutzen
			web_reader->send_http_get_request(
				cfg->wechselrichter_host,
				cfg->wechselrichter_port,
				"/components/inverter/readable"
			);
			while(web_reader->read_next_block_to_buffer()) {
				_detaildaten_einsetzen(elektroanlage);
			}
			if(!(findings & 0b1111'1111) && !(findings2 & 0b0000'0111)) {
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
