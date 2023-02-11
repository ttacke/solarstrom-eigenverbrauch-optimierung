#pragma once

namespace Local {
	class Config {
	public:
		const int log_baud = 9600;

		const char* wlan_ssid = "[WLAN-SSID]";
		const char* wlan_pwd = "[WLAN-PASSWORT]";

		const int webserver_port = 80;

		// Fronius Gen24 8kW mit FromiusSmartMeter
		const char* wechselrichter_host = "192.168.0.14";
		const int wechselrichter_port = 80;

		const char* accuweather_api_key = "[API-KEY VON developer.accuweather.com]";
		const int accuweather_location_id = 1008813;

		const int maximale_solarstrahlung_in_w_pro_m2 = 1000;// Quelle https://www.weltderphysik.de/gebiet/technik/energie/solarenergie/sonnenenergie/
		const int maximale_solarstrahlung_pro_tag_in_w_pro_m2 = 7000;// geraten; max Tageslaenge 10,5h bei 1000W/m2 minus Glockenkurve
		// Ggf hiermit sinniger? Oder selber messen. https://re.jrc.ec.europa.eu/pvg_tools/en/
	};
}
