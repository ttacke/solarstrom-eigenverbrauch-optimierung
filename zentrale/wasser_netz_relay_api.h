#pragma once
#include "base_api.h"

namespace Local {
	class WasserNetzRelayAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	public:
		bool ist_aktiv() {
			return _ist_an(cfg->wasser_ueberladen_host, cfg->wasser_ueberladen_port);
		}

//		void schalte(bool ein) {
//			_schalte(cfg->wasser_ueberladen_host, cfg->wasser_ueberladen_port, ein);
//		}
	};
}
