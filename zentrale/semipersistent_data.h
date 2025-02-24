#pragma once

// Daten werden hier, nicht im FS gespeichert, weil die SD-Karte sonst innerhalb weniger Jahre kaputt geht
// Die Daten sind nur im RAM, gehen bei jedem Restart also verloren
namespace Local::SemipersistentData {
	int roller_relay_zustand_seit = 0;
}
