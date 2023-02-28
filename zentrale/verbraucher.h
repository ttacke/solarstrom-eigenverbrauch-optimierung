#pragma once

namespace Local {
	class Verbraucher {
	public:
		enum class Ladestatus {force, solar, off};

		// TODO Teil von "elektro_anlage"? Eigentlich ja!
		bool wasser_ueberladen_ist_an = false;
		bool heizung_ueberladen_ist_an = false;
		Ladestatus auto_ladestatus = Local::Verbraucher::Ladestatus::off;
		int auto_ladeleistung_in_w = 0;
		Ladestatus roller_ladestatus = Local::Verbraucher::Ladestatus::off;
		int roller_ladeleistung_in_w = 0;
	};
}
