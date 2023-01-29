#pragma once

namespace Local {
	class Config {
	public:
		const int log_baud = 115200;
		const int webserver_port = 80;
		// Fronius Gen24 8kW
		const char* wechselrichter_data_url = (char*) "http://[WECHLELRICHTER-IP]/status/powerflow";
		// FromiusSmartMeter via Fronius Gen24 8kW
		const char* smartmeter_data_url = (char*) "http://[WECHLELRICHTER-IP]/components/PowerMeter/readable";
		// Die Nummer wird in der smartmeter_data_url geliefert und ist fix, taucht aber sonst nirgends auf
		const char* smartmeter_id = (char*) "[SMARTMETER-ID]";
		const char* wlan_ssid = (char*) "[WLAN-SSID]";
		const char* wlan_pwd = (char*) "[WLAN-PASSWORT]";
		// In welchen Zeitabstaenden (in Sekunden) soll die Anzeige aktualisiert werden
		const int refresh_display_interval = 60;
		// LocationID ermitteln via https://developer.accuweather.com/accuweather-locations-api/apis/get/locations/v1/search
		const char* wetter_stundenvorhersage_url = "http://dataservice.accuweather.com/forecasts/v1/hourly/12hour/[LOCATION-ID]?apikey=[API-KEY]&language=de-de&details=true&metric=true";
		const char* wetter_tagesvorhersage_url   = "http://dataservice.accuweather.com/forecasts/v1/daily/5day/[LOCATION-ID]?apikey=[API-KEY]&language=de-de&details=true&metric=true";
	};
}
