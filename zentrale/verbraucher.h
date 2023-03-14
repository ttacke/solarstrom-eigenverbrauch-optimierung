#pragma once

namespace Local {
	class Verbraucher {
	protected:
		int _gib_listen_maximum(int* liste, int length) {
			int max = liste[0];
			for(int i = 1; i < length; i++) {
				if(liste[i] > max) {
					max = liste[i];
				}
			}
			return max;
		}

	public:
		enum class Ladestatus {force, solar, off};

		bool wasser_relay_ist_an = false;
		int wasser_relay_zustand_seit = 0;
		float wasser_leistung_ist = 0;
		float wasser_leistung_soll = 0;

		bool heizung_relay_ist_an = false;
		int heizung_relay_zustand_seit = 0;
		float heizung_leistung_ist = 0;
		float heizung_leistung_soll = 0;

		Ladestatus auto_ladestatus = Local::Verbraucher::Ladestatus::off;
		int auto_benoetigte_ladeleistung_in_w = 0;
		int aktuelle_auto_ladeleistung_in_w = 0;
		int auto_ladeleistung_log_in_w[5];
		bool auto_relay_ist_an = false;
		int auto_relay_zustand_seit = 0;
		float auto_leistung_ist = 0;
		float auto_leistung_soll = 0;

		Ladestatus roller_ladestatus = Local::Verbraucher::Ladestatus::off;
		int roller_benoetigte_ladeleistung_in_w = 0;
		int aktuelle_roller_ladeleistung_in_w = 0;
		int roller_ladeleistung_log_in_w[5];
		bool roller_relay_ist_an = false;
		int roller_relay_zustand_seit = 0;
		float roller_leistung_ist = 0;
		float roller_leistung_soll = 0;

		int aktueller_verbrauch_in_w = 0;
		int verbrauch_log_in_w[5];
		int aktuelle_erzeugung_in_w = 0;
		int erzeugung_log_in_w[30];
		int aktueller_akku_ladenstand_in_promille = 0;
		int akku_ladestandsvorhersage_in_promille[12];
		int solarerzeugung_in_w = 0;
		int zeitpunkt_sonnenuntergang = 0;

		bool akku_lauft_potentiell_ueber() {
			for(int i = 0; i < 12; i++) {
				if(akku_ladestandsvorhersage_in_promille[i] > 1000) {
					return true;
				}
			}
			return false;
		}

		void set_auto_soll_ist_leistung(char* buffer) {
			if(auto_ladestatus == Local::Verbraucher::Ladestatus::solar) {
				sprintf(buffer, "%.1f/%.1f", auto_leistung_soll, auto_leistung_ist);
			} else {
				sprintf(buffer, "-");
			}
		}

		void set_roller_soll_ist_leistung(char* buffer) {
			if(roller_ladestatus == Local::Verbraucher::Ladestatus::solar) {
				sprintf(buffer, "%.1f/%.1f", roller_leistung_soll, roller_leistung_ist);
			} else {
				sprintf(buffer, "-");
			}
		}

		void set_wasser_soll_ist_leistung(char* buffer) {
			sprintf(buffer, "%.1f/%.1f", wasser_leistung_soll, wasser_leistung_ist);
		}

		void set_heizung_soll_ist_leistung(char* buffer) {
			sprintf(buffer, "%.1f/%.1f", heizung_leistung_soll, heizung_leistung_ist);
		}

		bool solarerzeugung_ist_aktiv() {
			return solarerzeugung_in_w > 20;
		}

		int gib_genutzte_auto_ladeleistung_in_w() {
			return _gib_listen_maximum(auto_ladeleistung_log_in_w, 5);
		}

		int gib_genutzte_roller_ladeleistung_in_w() {
			return _gib_listen_maximum(roller_ladeleistung_log_in_w, 5);
		}

		int gib_beruhigten_ueberschuss_in_w() {
			int erzeugung = aktuelle_erzeugung_in_w;
			for(int i = 0; i < 30; i++) {
				erzeugung += erzeugung_log_in_w[i];
			}

			int max_verbrauch = aktueller_verbrauch_in_w;
			for(int i = 0; i < 5; i++) {
				if(max_verbrauch < verbrauch_log_in_w[i]) {
					max_verbrauch = verbrauch_log_in_w[i];
				}
			}
			return round((erzeugung / 31) - max_verbrauch);
		}

		void set_log_data_a(char* buffer) {
			sprintf(
				buffer,
				"va2,%s,%s,%d,%d,%d,%d,%d,%d,%d,%s",
				(
					auto_ladestatus == Local::Verbraucher::Ladestatus::force
					? "force"
						: auto_ladestatus == Local::Verbraucher::Ladestatus::solar
						? "solar" : "off"
				),
				auto_relay_ist_an ? "an" : "aus",
				auto_benoetigte_ladeleistung_in_w,
				aktuelle_auto_ladeleistung_in_w,
				gib_genutzte_auto_ladeleistung_in_w(),
				wasser_relay_ist_an ? "an" : "aus"
			);
		}

		void set_log_data_b(char* buffer) {
			sprintf(
				buffer,
				"vb2,%s,%s,%d,%d,%d,%d,%d,%d,%d,%s",
				(
					roller_ladestatus == Local::Verbraucher::Ladestatus::force
					? "force"
						: roller_ladestatus == Local::Verbraucher::Ladestatus::solar
						? "solar" : "off"
				),
				roller_relay_ist_an ? "an" : "aus",
				roller_benoetigte_ladeleistung_in_w,
				aktuelle_roller_ladeleistung_in_w,
				gib_genutzte_roller_ladeleistung_in_w(),
				heizung_relay_ist_an ? "an" : "aus"
			);
		}
	};
}
