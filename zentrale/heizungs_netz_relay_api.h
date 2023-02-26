#pragma once
#include "base_api.h"

namespace Local {
	class HeizungsNetzRelayAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	public:
		bool ist_aktiv() {
			return _ist_an(cfg->heizung_ueberladen_host, cfg->heizung_ueberladen_port);
		}

//		void schalte(bool ein) {
//			_schalte(cfg->heizung_ueberladen_host, cfg->heizung_ueberladen_port, ein);
//		}
	};
}
