#pragma once

namespace Local {
	class Config {
	public:
		const int log_baud = 115200;
		// Fronius Gen24 8kW
		const char* wechselrichter_data_url = (char*) "http://192.168.0.14/status/powerflow";
		// FromiusSmartMeter via Fronius Gen24 8kW
		const char* smartmeter_data_url = (char*) "http://192.168.0.14/components/PowerMeter/readable";
		// Die Nummer wird in der smartmeter_data_url geliefert und ist fix, taucht aber sonst nirgends auf
		const char* smartmeter_id = (char*) "16711680";
		//const int speicher_groesse = 7650;// in Wh
		const char* wlan_ssid = (char*) "[BITTE-ANGEBEN]";
		const char* wlan_pwd = (char*) "[BITTE-ANGEBEN]";
		// In welchen Zeitabstaenden (in Sekunden) soll die Anzeige aktualisiert werden
		const int refresh_display_interval = 60;
	};
}
