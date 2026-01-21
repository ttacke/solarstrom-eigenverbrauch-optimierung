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
		const int roller_relay_port = 80;
		const int roller_relay_version = 1;
		const char* roller_aussen_relay_host = "192.168.0.34";
		const int roller_aussen_relay_port = 80;
		const int roller_aussen_relay_version = 3;
		const char* wasser_relay_host = "192.168.0.31";
		const int wasser_relay_port = 80;
		const char* auto_relay_host = "192.168.0.32";
		const int auto_relay_port = 80;
		const char* heizung_relay_host = "192.168.0.33";
		const int heizung_relay_port = 80;
		const char* heizstab_relay_host = "192.168.0.38";
		const int heizstab_relay_port = 80;
		const int heizstab_relay_version = 3;
		const char* heizung_luftvorwaermer_relay_host = "192.168.0.35";
		const int heizung_luftvorwaermer_relay_port = 80;
		const int heizung_luftvorwaermer_relay_version = 1;
		const char* wasser_begleitheizung_relay_host = "192.168.0.36";
		const int wasser_begleitheizung_relay_port = 80;
		const int wasser_begleitheizung_relay_version = 1;
		const char* wasser_wp_relay_host = "192.168.0.39";
		const int wasser_wp_relay_port = 80;
		const int wasser_wp_relay_version = 3;

		const int roller_benoetigte_leistung_gering_in_w = 420;
		const int roller_benoetigte_leistung_hoch_in_w = 840;
		const int roller_min_schaltzeit_in_min = 10;
		const int auto_benoetigte_leistung_in_w = 2300;
		const int auto_min_schaltzeit_in_min = 10;
		const int auto_benutzter_leiter = 1; // 1, 2, 3
		const int wasser_benoetigte_leistung_in_w = 420;
		const int wasser_min_schaltzeit_in_min = 60;
		const int heizung_benoetigte_leistung_in_w = 420;
		const int heizung_min_schaltzeit_in_min = 60;
		const int heizstab_benoetigte_leistung_in_w = 1500;
		const int heizstab_einschalt_differenzwert = 542;// 540 ist das Minimum
		const int heizstab_ausschalt_differenzwert = 545;// ca Minimum+3
		const float heizung_luftvorwaermer_zuluft_einschalttemperatur = 15.3;
		const float heizung_luftvorwaermer_zuluft_ausschalttemperatur = 15.5;
		const float heizung_luftvorwaermer_abluft_einschalttemperatur = 6.8;
		const float heizung_luftvorwaermer_abluft_ausschalttemperatur = 7.0;
		const float heizung_max_ablufttemperatur_wenn_aktiv = 10.0;
		const int heizung_luftvorwaermer_benoetigte_leistung_in_w = 850;
		const int wasser_begleitheizung_benoetigte_leistung_in_w = 500;// Einschaltwert, sinkt rapide auf ~120

		const int grundverbrauch_in_w_pro_h_sommer = 330;
		const int grundverbrauch_in_w_pro_h_winter = 645;
		const float solarstrahlungs_vorhersage_umrechnungsfaktor_sommer = 4.96;// 90% von 5.51
		const float solarstrahlungs_vorhersage_umrechnungsfaktor_winter = 5.63;// 90% von 6.25

		const int akku_groesse_in_wh = 7680;
		const int akku_zielladestand_in_promille = 900;// sollte <= maxSOC-Begrenzung sein
		const int akku_zielladestand_fuer_ueberladen_in_promille = 980;// sollte ~ maxSOC-Begrenzung sein
		const int ueberladen_hysterese_in_promille = 40;
		const int akku_ladestand_in_promille_fuer_erzwungenes_ueberladen = 780;
		const int akku_ladestand_in_promille_fuer_erzwungenes_laden = 760;
		const int nicht_laden_unter_akkuladestand_in_promille = 200;
		const int minimal_im_tagesgang_erreichbarer_akku_ladestand_in_promille = 100;
		const int frueh_leeren_starte_in_stunde_utc = 4;

		const int webserver_port = 80;

		// Fronius Gen24 8kW mit FromiusSmartMeter
		const char* wechselrichter_host = "192.168.0.14";
		const int wechselrichter_port = 80;
		const int maximale_wechselrichterleistung_in_w = 7000;
		const int maximaler_netzbezug_ausschaltgrenze_in_w = 6500;// ((59kVA (Syna) / 12) + 4.6kVA (lasterhoehung)) * 0,95 (VDE-AR-N 4100) = 9000 W
		const int einschaltreserve_in_w = 300;
		const int ladestatus_force_dauer = 12 * 3600;//12h
		const float solarmodul_flaeche = 50;// m²
		// TODO aufteilund dach#1 und 2 hier abbilden, damit bei der Aufteilung das benutzt werden kann
		// https://de.wikipedia.org/wiki/Spitzenlast
		// UTC = MEZ - 1; -1 = ignorieren
		const int winterladen_zwangspausen_utc[12] = {10,11,12,-1,15,16,17,-1,-1,-1,-1,-1};

		const float wettervorhersage_lat = 50.00;
		const float wettervorhersage_lon = 7.95;
		const int wettervorhersage_dach1_neigung_in_grad = 35;
		const int wettervorhersage_dach1_ausrichtung_azimuth = 30;
		const int wettervorhersage_dach2_neigung_in_grad = 35;
		const int wettervorhersage_dach2_ausrichtung_azimuth = 150;
		const float stundenwert_anpassung = solarmodul_flaeche / 35; // 60=vorhersage:6h bis voll, real:4h bis voll; 40 = minimal zu niedrig ~1h daneben
		const float tageswert_anpassung = solarmodul_flaeche * 9.5;// 10 war zu hoch, 7.5 zu niedrig, 8,5 zu niedrig
	};
}
