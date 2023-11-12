#pragma once

namespace Local {
	class Config {
	public:
		const int log_baud = 9600;

		const char* network_connection_check_host = "192.168.2.1";
		const char* network_connection_check_path = "/unknownpath";
//		const char* network_connection_check_content = "body";
		const char* wlan_ssid = "[WLAN-SSID]";
		const char* wlan_pwd = "[WLAN-PASSWORT]";

		const char* roller_relay_host = "192.168.2.30";
		int roller_relay_port = 80;
		const char* roller_aussen_relay_host = "192.168.2.34";
		int roller_aussen_relay_port = 80;
		const char* wasser_relay_host = "192.168.2.31";
		int wasser_relay_port = 80;
		const char* auto_relay_host = "192.168.2.32";
		int auto_relay_port = 80;
		const char* heizung_relay_host = "192.168.2.33";
		const int heizung_relay_port = 80;
		const char* verbrennen_relay_host = "192.168.2.36";
		const int verbrennen_relay_port = 80;

		int roller_benoetigte_leistung_gering_in_w = 420;
		int roller_benoetigte_leistung_hoch_in_w = 840;
		int roller_min_schaltzeit_in_min = 10;
		int auto_benoetigte_leistung_in_w = 3680;
		int auto_min_schaltzeit_in_min = 10;
		int wasser_benoetigte_leistung_in_w = 420;
		int wasser_min_schaltzeit_in_min = 60;
		int heizung_benoetigte_leistung_in_w = 420;
		int heizung_min_schaltzeit_in_min = 60;
		int verbrennen_min_schaltzeit_in_min = 5;

		// TODO diesen configwert als SD_File ablegen, dann kann das von außen aktualisiert werden
		int grundverbrauch_in_w_pro_h = 263;
		// TODO diesen configwert als SD_File ablegen, dann kann das von außen aktualisiert werden
		// TODO Monatsweise setzen, denn das ist jeden Monat anders
		float solarstrahlungs_vorhersage_umrechnungsfaktor = 4.65;
		int akku_groesse_in_wh = 7680;
		int akku_zielladestand_in_promille = 800;
		int minimaler_akku_ladestand = 200;
		int frueh_leeren_starte_in_stunde_utc = 4;

		const int webserver_port = 80;

		// Fronius Gen24 8kW mit FromiusSmartMeter
		const char* wechselrichter_host = "192.168.2.14";
		const int wechselrichter_port = 80;

		const char* accuweather_api_key = "[API-KEY VON developer.accuweather.com]";
		const int accuweather_location_id = 1008813;// Ermittelt via https://developer.accuweather.com/accuweather-locations-api/apis/get/locations/v1/search
	};
}
