#pragma once

namespace Local {
	class ElektroAnlage {
	public:
		int netzbezug_in_wh;// + = bezug, - = einspeisung
		int solarakku_zuschuss_in_wh;// + = entladung, - = ladung
		unsigned int solarerzeugung_in_wh;
		unsigned int stromverbrauch_in_wh;
		unsigned int solarakku_ladestand_in_promille;
		bool solarakku_ist_an = false;
		int l1_strom_ma;
		int l2_strom_ma;
		int l3_strom_ma;

		int gib_ueberschuss_in_wh() {
			int ueberschuss = (netzbezug_in_wh * -1);
			if(solarakku_zuschuss_in_wh < 0) {// Wenn der laedt...
				ueberschuss -= solarakku_zuschuss_in_wh;// die Ladeleistung zum Ueberschuss zaehlen
			}
			return ueberschuss;
		}

		int max_i_in_ma() {
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

		String gib_log_zeile() {
			return "Ev1;"
				+ (String) netzbezug_in_wh + ";"
				+ (String) solarakku_zuschuss_in_wh + ";"
				+ (String) solarerzeugung_in_wh + ";"
				+ (String) stromverbrauch_in_wh + ";"
				+ (String) solarakku_ladestand_in_promille + ";"
				+ (String) l1_strom_ma + ";"
				+ (String) l2_strom_ma + ";"
				+ (String) l3_strom_ma
			;
		}
	};
}
