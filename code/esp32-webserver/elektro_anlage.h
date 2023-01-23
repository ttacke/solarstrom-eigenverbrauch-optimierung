#pragma once

namespace Local {
	class ElektroAnlage {
	public:
		int solar_wh = 0;
		int netz_wh = 0;
		int solarakku_wh = 0;
		int verbraucher_wh = 0;
		int solarakku_ladestand_prozent = 0;
		bool solarakku_ist_an = false;
		int l1_strom_ma = 0;
		int l2_strom_ma = 0;
		int l3_strom_ma = 0;

		int max_i_ma() {
			if(l1_strom_ma > l2_strom_ma && l1_strom_ma > l3_strom_ma) {
				return l1_strom_ma;
			} else if(l2_strom_ma > l3_strom_ma) {
				return l2_strom_ma;
			} else {
				return l3_strom_ma;
			}
		}

		int max_i_phase() {
			if(l1_strom_ma > l2_strom_ma && l1_strom_ma > l3_strom_ma) {
				return 1;
			} else if(l2_strom_ma > l3_strom_ma) {
				return 2;
			} else {
				return 3;
			}
		}
	};
}
