#pragma once

namespace Local {
	class ElektroAnlage {
	public:
		int solar_wh;
		int netz_wh;
		int solarakku_wh;
		int verbraucher_wh;
		int solarakku_ladestand_prozent;
		bool solarakku_ist_an = false;
		int l1_strom_ma;
		int l2_strom_ma;
		int l3_strom_ma;

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
