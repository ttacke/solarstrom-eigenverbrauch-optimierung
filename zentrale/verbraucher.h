#pragma once

namespace Local {
	class Verbraucher {
	public:
		enum class Ladestatus {force, solar, off};

		bool wasser_relay_ist_an = false;
		int wasser_relay_zustand_seit = 0;

		bool heizung_relay_ist_an = false;
		int heizung_relay_zustand_seit = 0;

		Ladestatus auto_ladestatus = Local::Verbraucher::Ladestatus::off;
		int auto_benoetigte_ladeleistung_in_w = 0;
		int aktuelle_auto_ladeleistung_in_w = 0;
		int auto_ladeleistung_log_in_w[5];
		bool auto_relay_ist_an = false;
		int auto_relay_zustand_seit = 0;

		Ladestatus roller_ladestatus = Local::Verbraucher::Ladestatus::off;
		int roller_benoetigte_ladeleistung_in_w = 0;
		int aktuelle_roller_ladeleistung_in_w = 0;
		int roller_ladeleistung_log_in_w[5];
		bool roller_relay_ist_an = false;
		int roller_relay_zustand_seit = 0;

		int aktueller_ueberschuss_in_w = 0;
		int ueberschuss_log_in_w[5];
		int aktueller_akku_ladenstand_in_promille = 0;
		int solarerzeugung_in_w = 0;
		int zeitpunkt_sonnenuntergang = 0;

		bool solarerzeugung_ist_aktiv() {
			return solarerzeugung_in_w > 20;
		}

		void set_log_data_a(char* buffer) {
			sprintf(
				buffer,
				"va1,%s,%s,%d,%d,%d,%d,%d,%d,%d,%s",
				(
					auto_ladestatus == Local::Verbraucher::Ladestatus::force
					? "force"
						: auto_ladestatus == Local::Verbraucher::Ladestatus::solar
						? "solar" : "off"
				),
				auto_relay_ist_an ? "an" : "aus",
				auto_benoetigte_ladeleistung_in_w,
				aktuelle_auto_ladeleistung_in_w,
				auto_ladeleistung_log_in_w[4],
				auto_ladeleistung_log_in_w[3],
				auto_ladeleistung_log_in_w[2],
				auto_ladeleistung_log_in_w[1],
				auto_ladeleistung_log_in_w[0],
				wasser_relay_ist_an ? "an" : "aus"
			);
		}

		void set_log_data_b(char* buffer) {
			sprintf(
				buffer,
				"vb1,%s,%s,%d,%d,%d,%d,%d,%d,%d,%s",
				(
					roller_ladestatus == Local::Verbraucher::Ladestatus::force
					? "force"
						: roller_ladestatus == Local::Verbraucher::Ladestatus::solar
						? "solar" : "off"
				),
				roller_relay_ist_an ? "an" : "aus",
				roller_benoetigte_ladeleistung_in_w,
				aktuelle_roller_ladeleistung_in_w,
				roller_ladeleistung_log_in_w[4],
				roller_ladeleistung_log_in_w[3],
				roller_ladeleistung_log_in_w[2],
				roller_ladeleistung_log_in_w[1],
				roller_ladeleistung_log_in_w[0],
				heizung_relay_ist_an ? "an" : "aus"
			);
		}
	};
}
