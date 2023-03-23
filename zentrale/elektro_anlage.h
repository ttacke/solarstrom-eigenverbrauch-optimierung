#pragma once

namespace Local {
	class ElektroAnlage {
	public:
		int netzbezug_in_w = 0;// + = bezug, - = einspeisung
		int solarakku_zuschuss_in_w = 0;// + = entladung, - = ladung
		unsigned int solarerzeugung_in_w = 0;
		unsigned int stromverbrauch_in_w = 0;
		unsigned int solarakku_ladestand_in_promille = 0;
		bool solarakku_ist_an = false;
		int l1_strom_ma = 0;
		int l2_strom_ma = 0;
		int l3_strom_ma = 0;
		int l1_solarstrom_ma = 0;
		int leistungsanteil_pv1 = 0;
		int leistungsanteil_pv2 = 0;
		// TODO wenn aktiv, dann: 110% AkkuZielwert statt 80 bzw. 100 && Kein Forced laden!
		bool ersatzstrom_ist_aktiv = false;

		int gib_ueberschuss_in_w() {
			return solarerzeugung_in_w - stromverbrauch_in_w;
		}

		int gib_anteil_pv1_in_prozent() {
			if(!leistungsanteil_pv1 && !leistungsanteil_pv2) {
				return 0;
			}
			return round((float) leistungsanteil_pv1 * 100 / (float) (leistungsanteil_pv2 + leistungsanteil_pv1));
		}

		void write_log_data(Local::Service::FileWriter& file_writer) {
			file_writer.write_formated(
				"e2,%d,%d,%d,%d,%d,%d,%d,%d,%d,%i",
				netzbezug_in_w,
				solarakku_zuschuss_in_w,
				solarerzeugung_in_w,
				stromverbrauch_in_w,
				solarakku_ladestand_in_promille,
				l1_strom_ma,
				l2_strom_ma,
				l3_strom_ma,
				gib_anteil_pv1_in_prozent(),
				ersatzstrom_ist_aktiv ? 1 : 0
			);
		}
	};
}
