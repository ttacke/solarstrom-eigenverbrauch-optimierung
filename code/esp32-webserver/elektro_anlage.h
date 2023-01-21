#pragma once
#include <ArduinoJson.h>

class ElektroAnlage {
protected:
	int solar_wh;
	int netz_wh;
	int verbraucher_wh;
	int solarakku_ladestand_prozent;
	bool solarakku_ist_an;
	int maximale_stromstaerke_ma;
	int phase_der_maximalen_stromstaerke;
public:
	ElektroAnlage() {
	}
};
