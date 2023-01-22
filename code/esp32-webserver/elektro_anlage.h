#pragma once

class ElektroAnlage {
public:
	int solar_wh;
	int netz_wh;
	int solarakku_wh;
	int verbraucher_wh = 12;
	int solarakku_ladestand_prozent;
	bool solarakku_ist_an;
	int maximale_stromstaerke_ma;
	int phase_der_maximalen_stromstaerke;
	ElektroAnlage() {
	}
};
