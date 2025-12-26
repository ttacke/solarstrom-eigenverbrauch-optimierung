#pragma once

// Daten werden hier, nicht im FS gespeichert, weil die SD-Karte sonst innerhalb weniger Jahre kaputt geht
// Die Daten sind nur im RAM, gehen bei jedem Restart also verloren
namespace Local::SemipersistentData {
	int roller_relay_zustand_seit = 0;
	int heizung_relay_zustand_seit = 0;
	int wasser_relay_zustand_seit = 0;
	int auto_relay_zustand_seit = 0;
	int heizstabbetrieb_letzter_zustand = true;
	int wettervorhersage_letzter_abruf = 0;
	int auto_ladeleistung_log_in_w[5] = {0,0,0,0,0};
	int roller_ladeleistung_log_in_w[5] = {0,0,0,0,0};
	int verbrauch_log_in_w[5] = {0,0,0,0,0};
	int erzeugung_log_in_w[30] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	int frueh_leeren_auto_zuletzt_gestartet_an_timestamp = 0;
	bool frueh_leeren_auto_ist_aktiv = false;
	int frueh_leeren_roller_zuletzt_gestartet_an_timestamp = 0;
	bool frueh_leeren_roller_ist_aktiv = false;
}
