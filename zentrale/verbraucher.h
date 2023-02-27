#pragma once

namespace Local {
	class Verbraucher {
	public:
		enum class Ladestatus {force, solar, off};

		bool wasser_ueberladen_ist_an = false;
		bool heizung_ueberladen_ist_an = false;
		Ladestatus auto_laden_status = Local::Verbraucher::Ladestatus::off;
		int auto_ladeleistung_in_w = 2000;// 3700
		Ladestatus roller_laden_status = Local::Verbraucher::Ladestatus::off;
		int roller_ladeleistung_in_w = 840;// 420
	};
}
