#pragma once

namespace Local {
	class Config {
	public:
		const int log_baud = 9600;

		const char* wlan_ssid = (char*) "[WLAN-SSID]";
		const char* wlan_pwd = (char*) "[WLAN-PASSWORT]";

		const int webserver_port = 80;

		const char* wechselrichter_host = (char*) "192.168.0.14";
		const int wechselrichter_port = 80;

// TODO in Leser schieben!
		// Fronius Gen24 8kW
		const char* wechselrichter_data_request_uri = (char*) "http://[WECHLELRICHTER-IP]/status/powerflow";
		// FromiusSmartMeter via Fronius Gen24 8kW
		const char* wechselrichter_smartmeter_data_request_uri = (char*) "/components/PowerMeter/readable";

// ---- alt
		// TODO in Leser verschieben, aber locationID und APIKey auslagern
		const char* wetter_stundenvorhersage_url = "http://dataservice.accuweather.com/forecasts/v1/hourly/12hour/[LOCATION-ID]?apikey=[API-KEY]&language=de-de&details=true&metric=true";
		// TODO nutzen! const char* wetter_tagesvorhersage_url   = "http://dataservice.accuweather.com/forecasts/v1/daily/5day/[LOCATION-ID]?apikey=[API-KEY]&language=de-de&details=true&metric=true";
	};
}
