#pragma once

namespace Local {
	class Verbraucher {
	public:
		enum class Ladestatus {force, solar, off};

		bool wasser_ueberladen_ist_an = false;

		bool heizung_ueberladen_ist_an = false;

		Ladestatus auto_ladestatus = Local::Verbraucher::Ladestatus::off;
		int auto_benoetigte_leistung_in_w = 0;
		int aktuelle_auto_ladeleistung_in_w = 0;
		int auto_leistung_log_in_w[5];

		Ladestatus roller_ladestatus = Local::Verbraucher::Ladestatus::off;
		int roller_ladeleistung_in_w = 0;
		int aktuelle_roller_ladeleistung_in_w = 0;
		int roller_leistung_log_in_w[5];

		int aktueller_ueberschuss_in_w = 0;
		int ueberschuss_log_in_w[5];
		int aktueller_akku_ladenstand_in_promille = 0;
	};
}
