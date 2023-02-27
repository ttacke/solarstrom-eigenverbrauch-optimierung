#pragma once
#include "base_api.h"

namespace Local {
	class WasserNetzRelayAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		bool relay_ist_an = false;

	public:
		bool ist_aktiv() {
			return relay_ist_an;
		}

//		void schalte(bool ein) {
//			_schalte(cfg->wasser_ueberladen_host, cfg->wasser_ueberladen_port, ein);
//		}

		void heartbeat(int now_timestamp) {
			relay_ist_an = _ist_an(cfg->wasser_ueberladen_host, cfg->wasser_ueberladen_port);
			// TODO wird 1x die Minute gestartet
			// TODO Alle 4 Relays in eins stopfen, denn die müssen voneinander wissen
			// Es soll ja ein ab und ein angeschaltet werden (nach minLaufzeitRegeln)
			// wenn es besser in die Leistund passt
		}
	};
}
