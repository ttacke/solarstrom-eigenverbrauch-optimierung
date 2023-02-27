#pragma once
#include "base_api.h"

namespace Local {
	class VerbraucherAPI: public BaseAPI {

	using BaseAPI::BaseAPI;

	protected:

	public:
		void daten_holen_und_einsetzen(Local::Verbraucher& verbraucher) {

		}

		void setze_roller_ladestatus(Local::Verbraucher::Ladestatus status) {
		}

		void setze_auto_ladestatus(Local::Verbraucher::Ladestatus status) {
		}

		void wechsle_auto_ladeleistung() {
			// TODO
		}

		void wechsle_roller_ladeleistung() {
			// TODO
		}
	};
}
