#pragma once
#include "base_leser.h"

namespace Local {
	class SmartmeterLeser: public BaseLeser {

	using BaseLeser::BaseLeser;
	protected:
		void _daten_extrahieren_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			if(web_client->find_in_content((char*) "\"SMARTMETER_CURRENT_01_F64\" *: *([0-9.]+)[^0-9]")) {
				elektroanlage.l1_strom_ma = round(atof(web_client->finding_buffer) * 1000);
			}
			if(web_client->find_in_content((char*) "\"SMARTMETER_CURRENT_02_F64\" *: *([0-9.]+)[^0-9]")) {
				elektroanlage.l2_strom_ma = round(atof(web_client->finding_buffer) * 1000);
			}
			if(web_client->find_in_content((char*) "\"SMARTMETER_CURRENT_03_F64\" *: *([0-9.]+)[^0-9]")) {
				elektroanlage.l3_strom_ma = round(atof(web_client->finding_buffer) * 1000);
			}
		}

	public:
		void daten_holen_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			web_client->send_http_get_request(
				cfg->wechselrichter_host,
				cfg->wechselrichter_port,
				cfg->wechselrichter_smartmeter_data_request_uri
			);
			while(web_client->read_next_block_to_buffer()) {
				_daten_extrahieren_und_einsetzen(elektroanlage);
			}
		}
	};
}
