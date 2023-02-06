#pragma once
#include "base_leser.h"

namespace Local {
	class SmartmeterLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	protected:
		std::uint8_t findings;

		void _daten_extrahieren_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			if(web_client->find_in_content((char*) "\"SMARTMETER_CURRENT_01_F64\" *: *([-0-9.]+)[^0-9]")) {
				elektroanlage.l1_strom_ma = round(atof(web_client->finding_buffer) * 1000);
				findings |= 0b0000'0001;
			}
			if(web_client->find_in_content((char*) "\"SMARTMETER_CURRENT_02_F64\" *: *([-0-9.]+)[^0-9]")) {
				elektroanlage.l2_strom_ma = round(atof(web_client->finding_buffer) * 1000);
				findings |= 0b0000'0010;
			}
			if(web_client->find_in_content((char*) "\"SMARTMETER_CURRENT_03_F64\" *: *([-0-9.]+)[^0-9]")) {
				elektroanlage.l3_strom_ma = round(atof(web_client->finding_buffer) * 1000);
				findings |= 0b0000'0100;
			}
		}

	public:
		void daten_holen_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			web_client->send_http_get_request(
				cfg->wechselrichter_host,
				cfg->wechselrichter_port,
				"/components/PowerMeter/readable"
			);
			findings = 0b0000'0000;
			while(web_client->read_next_block_to_buffer()) {
				_daten_extrahieren_und_einsetzen(elektroanlage);
			}
			if(findings & 0b0000'0111) {
				elektroanlage.smartmeterdaten_sind_valide = true;
			} else {
				elektroanlage.smartmeterdaten_sind_valide = false;
			}
		}
	};
}
