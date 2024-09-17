#pragma once

namespace Local {
	class Config {
	public:
		const int log_baud = 9600;

		const char* network_connection_check_host = "192.168.0.1";
		const char* network_connection_check_path = "/unknownpath";
//		const char* network_connection_check_content = "body";
		const char* internet_connection_check_host = "192.168.0.1";
		const char* internet_connection_check_path = "/unknownpath";

		const char* wlan_ssid = "[WLAN-SSID]";
		const char* wlan_pwd = "[WLAN-PASSWORT]";

		const char* roller_relay_host = "192.168.0.30";
		int roller_relay_port = 80;
		const char* roller_aussen_relay_host = "192.168.0.34";
		int roller_aussen_relay_port = 80;
		const char* wasser_relay_host = "192.168.0.31";
		int wasser_relay_port = 80;
		const char* auto_relay_host = "192.168.0.32";
		int auto_relay_port = 80;
		const char* heizung_relay_host = "192.168.0.33";
		const int heizung_relay_port = 80;
//		const char* router_neustart_relay_host = "192.168.0.36";
//		const int router_neustart_relay_port = 80;

		int roller_benoetigte_leistung_gering_in_w = 420;
		int roller_benoetigte_leistung_hoch_in_w = 840;
		int roller_min_schaltzeit_in_min = 10;
		int auto_benoetigte_leistung_in_w = 2990;
		int auto_min_schaltzeit_in_min = 10;
		int wasser_benoetigte_leistung_in_w = 420;
		int wasser_min_schaltzeit_in_min = 60;
		int heizung_benoetigte_leistung_in_w = 420;
		int heizung_min_schaltzeit_in_min = 60;

		int grundverbrauch_in_w_pro_h_sommer = 350;
		int grundverbrauch_in_w_pro_h_winter = 550;
		float solarstrahlungs_vorhersage_umrechnungsfaktor_sommer = 4.7;// 90% von 5.23
		float solarstrahlungs_vorhersage_umrechnungsfaktor_winter = 7.0;// 90% von 7.87
		int akku_groesse_in_wh = 7680;
		int akku_zielladestand_in_promille = 750;
		int nicht_laden_unter_akkuladestand_in_promille = 400;
		int minimaler_akku_ladestand_in_promille = 300;
		int frueh_leeren_starte_in_stunde_utc = 4;

		const int webserver_port = 80;

		// Fronius Gen24 8kW mit FromiusSmartMeter
		const char* wechselrichter_host = "192.168.0.14";
		const int wechselrichter_port = 80;
		const int maximale_wechselrichterleistung_in_w = 7000;
		const int maximaler_netzbezug_ausschaltgrenze_in_w = 4670;// 59kVA (Syna) / 12 * 0,95 (VDE-AR-N 4100)
		const int einschaltreserve_in_w = 300;
		const int ladestatus_force_dauer = 12 * 3600;//12h
		// https://de.wikipedia.org/wiki/Spitzenlast
		// UTC = MEZ - 1; -1 = ignorieren
		int winterladen_zwangspausen_utc[12] = {10,11,12,-1,15,16,17,-1,-1,-1,-1,-1};

		const char* accuweather_api_key = "[API-KEY VON developer.accuweather.com]";
		const int accuweather_location_id = 1008813;// Ermittelt via https://developer.accuweather.com/accuweather-locations-api/apis/get/locations/v1/search
	};
}
