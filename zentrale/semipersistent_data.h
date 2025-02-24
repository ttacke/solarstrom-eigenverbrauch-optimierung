#pragma once

// Daten werden hier, nicht im FS gespeichert, weil die SD-Karte sonst innerhalb weniger Jahre kaputt geht
// Die Daten sind nur im RAM, gehen bei jedem Restart also verloren
namespace Local::SemipersistentData {
	int roller_relay_zustand_seit = 0;
	int heizung_relay_zustand_seit = 0;
	int wasser_relay_zustand_seit = 0;
	int auto_relay_zustand_seit = 0;
	int stunden_wettervorhersage_letzter_abruf = 0;
	int tages_wettervorhersage_letzter_abruf = 0;
}
