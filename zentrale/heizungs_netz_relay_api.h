#pragma once
#include "base_api.h"

namespace Local {
	class HeizungsNetzRelayAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:
		bool relay_ist_an = false;

	public:
		bool ist_aktiv() {
			return relay_ist_an;
		}

//		void schalte(bool ein) {
//			_schalte(cfg->heizung_ueberladen_host, cfg->heizung_ueberladen_port, ein);
//		}

		void heartbeat(int now_timestamp) {
			relay_ist_an = _ist_an(cfg->heizung_ueberladen_host, cfg->heizung_ueberladen_port);
			// TODO wird 1x die Minute gestartet
		}
	};
}
