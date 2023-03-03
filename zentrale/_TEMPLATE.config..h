#pragma once

namespace Local {
	class Config {
	public:
		const int log_baud = 9600;

		const char* wlan_ssid = "[WLAN-SSID]";
		const char* wlan_pwd = "[WLAN-PASSWORT]";

		const char* roller_relay_host = "192.168.2.30";
		int roller_relay_port = 80;
		const char* wasser_relay_host = "192.168.2.31";
		int wasser_relay_port = 80;
		const char* auto_relay_host = "192.168.2.32";
		int auto_relay_port = 80;
		const char* heizung_relay_host = "192.168.2.33";
		const int heizung_relay_port = 80;

		int roller_benoetigte_leistung_gering_in_w = 420;
		int roller_benoetigte_leistung_hoch_in_w = 840;
		int roller_min_schaltzeit_in_min = 10;
		int auto_benoetigte_leistung_gering_in_w = 1840;
		int auto_benoetigte_leistung_hoch_in_w = 3680;
		int auto_min_schaltzeit_in_min = 10;
		int wasser_benoetigte_leistung_in_w = 420;
		int wasser_min_schaltzeit_in_min = 30;
		int heizung_benoetigte_leistung_in_w = 420;
		int heizung_min_schaltzeit_in_min = 30;

		int lastmanagement_schalt_karenzzeit_in_min = 5;

		const int webserver_port = 80;

		// Fronius Gen24 8kW mit FromiusSmartMeter
		const char* wechselrichter_host = "192.168.2.14";
		const int wechselrichter_port = 80;

		const char* accuweather_api_key = "[API-KEY VON developer.accuweather.com]";
		const int accuweather_location_id = 1008813;// Ermittelt via https://developer.accuweather.com/accuweather-locations-api/apis/get/locations/v1/search

		const int maximale_solarstrahlung_in_w_pro_m2 = 1000;// Quelle https://www.weltderphysik.de/gebiet/technik/energie/solarenergie/sonnenenergie/
		const int maximale_solarstrahlung_pro_tag_in_w_pro_m2 = 7000;
	};
}
