#pragma once

namespace Local {
	class ElektroAnlage {
	public:
		// TODO "in_wh" ist verstaendlicher
		int solar_wh;
		int netz_wh;
		int solarakku_wh;
		int verbraucher_wh;
		int solarakku_ladestand_prozent;
		bool solarakku_ist_an = false;
		int l1_strom_ma;
		int l2_strom_ma;
		int l3_strom_ma;

		int gib_ueberschuss_in_wh() {
			int ueberschuss = 0;
			if(solarakku_wh < 0) {
				ueberschuss += solarakku_wh;
			}
			if(netz_wh < 0) {
				ueberschuss += netz_wh;
			}
			return ueberschuss * -1;
		}

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
