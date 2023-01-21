class Config {
public:
	const static int log_baud = 9600;
	// Fronius Gen24 8kW
	constexpr static char* wechselrichter_data_url = (char*) "http://192.168.0.14/status/powerflow";
	// FromiusSmartMeter via Fronius Gen24 8kW
	constexpr static char* smartmeter_data_url = (char*) "http://192.168.0.14/components/PowerMeter/readable";
	// Die Nummer wird in der smartmeter_data_url geliefert und ist fix, taucht aber sonst nirgends auf
	constexpr static char* smartmeter_id = (char*) "16711680";
	constexpr static float speicher_groesse = 7650;// in Wh
	constexpr static char* wlan_ssid = (char*) "[BITTE-ANGEBEN]";
	constexpr static char* wlan_pwd = (char*) "[BITTE-ANGEBEN]";
	// In welchen Zeitabstaenden (in Sekunden) soll die Anzeige aktualisiert werden
	constexpr static char* refresh_display_interval = (char*) "1 * 60";
};
