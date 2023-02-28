#pragma once

namespace Local {
	class Config {
	public:
		const int log_baud = 9600;

		const char* wlan_ssid = "[WLAN-SSID]";
		const char* wlan_pwd = "[WLAN-PASSWORT]";

		const char* roller_laden_host = "192.168.2.30";
		int roller_laden_port = 80;
		const char* wasser_ueberladen_host = "192.168.2.31";
		int wasser_ueberladen_port = 80;
		const char* auto_laden_host = "192.168.2.32";
		int auto_laden_port = 80;
		const char* heizung_ueberladen_host = "192.168.2.33";
		const int heizung_ueberladen_port = 80;

		int roller_ladeleistung_gering_in_w = 420;
		int roller_ladeleistung_hoch_in_w = 840;
		int auto_ladeleistung_gering_in_w = 2200;
		int auto_ladeleistung_hoch_in_w = 3680;
		int wasser_ueberladeleistung_in_w = 420;
		int heizung_ueberladeleistung_in_w = 420;

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
