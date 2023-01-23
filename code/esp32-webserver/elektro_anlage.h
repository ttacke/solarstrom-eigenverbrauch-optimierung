#pragma once

namespace Local {
	class ElektroAnlage {
	public:
		int solar_wh;
		int netz_wh;
		int solarakku_wh;
		int verbraucher_wh;
		int solarakku_ladestand_prozent;
		bool solarakku_ist_an;
		int l1_strom_ma;
		int l2_strom_ma;
		int l3_strom_ma;
	};
}
