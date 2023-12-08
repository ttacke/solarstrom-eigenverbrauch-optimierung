#pragma once

namespace Local::Model {
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

		int _gib_genutzte_auto_ladeleistung_in_w() {
			return _gib_listen_maximum(auto_ladeleistung_log_in_w, 5);
		}

		int _gib_genutzte_roller_ladeleistung_in_w() {
			return _gib_listen_maximum(roller_ladeleistung_log_in_w, 5);
		}

	public:
		enum class Ladestatus {force, solar};

		bool wasser_relay_ist_an = false;
		bool wasser_lastschutz = false;
		int wasser_relay_zustand_seit = 0;

		bool heizung_relay_ist_an = false;
		bool heizung_lastschutz = false;
		int heizung_relay_zustand_seit = 0;

		Ladestatus auto_ladestatus = Local::Model::Verbraucher::Ladestatus::solar;
		int auto_ladestatus_seit = 0;
		int auto_benoetigte_ladeleistung_in_w = 0;
		int aktuelle_auto_ladeleistung_in_w = 0;
		int auto_ladeleistung_log_in_w[5];
		bool auto_relay_ist_an = false;
		bool auto_lastschutz = false;
		int auto_relay_zustand_seit = 0;

		Ladestatus roller_ladestatus = Local::Model::Verbraucher::Ladestatus::solar;
		int roller_ladestatus_seit = 0;
		int roller_benoetigte_ladeleistung_in_w = 0;
		int aktuelle_roller_ladeleistung_in_w = 0;
		int roller_ladeleistung_log_in_w[5];
		bool roller_relay_ist_an = false;
		bool roller_lastschutz = false;
		int roller_relay_zustand_seit = 0;

		int aktueller_verbrauch_in_w = 0;
		int verbrauch_log_in_w[5];
		int aktuelle_erzeugung_in_w = 0;
		int erzeugung_log_in_w[30];
		int aktueller_akku_ladenstand_in_promille = 0;
		int akku_ladestandsvorhersage_in_promille[48];
		int solarerzeugung_in_w = 0;
		int zeitpunkt_sonnenuntergang = 0;
		bool ersatzstrom_ist_aktiv = false;
		bool ladeverhalten_wintermodus = false;
		int netzbezug_in_w = 0;

		int gib_stundenvorhersage_akku_ladestand_als_fibonacci(int index) {
			int max_promille = 0;
			for(int i = 0; i < 4; i++) {
				int promille = akku_ladestandsvorhersage_in_promille[index * 4 + i];
				if(max_promille < promille) {
					max_promille += promille;
				}
			}
			int prozent = round((float) max_promille / 10);
			int a = 5;
			int b = 5;
			int tmp;
			for(int i = 0; i < 10; i++) {
				if(prozent <= b) {
					return i * 10;
				}
				tmp = b;
				b = a + b;
				a = tmp;
			}
			return 100;
		}

		bool akku_erreicht_ladestand_in_promille(int ladestand_in_promille) {
			for(int i = 0; i < 48; i++) {
				if(akku_ladestandsvorhersage_in_promille[i] >= ladestand_in_promille) {
					return true;
				}
			}
			return false;
		}

		bool akku_unterschreitet_ladestand_in_promille(int ladestand_in_promille) {
			for(int i = 0; i < 48; i++) {
				if(akku_ladestandsvorhersage_in_promille[i] < ladestand_in_promille) {
					return true;
				}
			}
			return false;
		}

		float akku_erreicht_ladestand_in_stunden(int ladestand_in_promille) {
			for(int i = 0; i < 48; i++) {
				if(akku_ladestandsvorhersage_in_promille[i] >= ladestand_in_promille) {
					return (float) i / 4;
				}
			}
			return 999;
		}

		bool solarerzeugung_ist_aktiv() {
			return solarerzeugung_in_w > 20;
		}

		bool auto_laden_ist_an() {
			if(
				auto_relay_ist_an
				&& _gib_genutzte_auto_ladeleistung_in_w() > (float) auto_benoetigte_ladeleistung_in_w * 0.8
			) {
				return true;
			}
			return false;
		}

		bool roller_laden_ist_an() {
			if(
				roller_relay_ist_an
				&& _gib_genutzte_roller_ladeleistung_in_w() > (float) roller_benoetigte_ladeleistung_in_w * 0.7
			) {
				return true;
			}
			return false;
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

		void write_log_data(Local::Service::FileWriter& file_writer) {
			file_writer.write_formated(
				"va3,%s,%s,%d,%d,%d,%s",
				(auto_ladestatus == Local::Model::Verbraucher::Ladestatus::force ? "force" : "solar"),
				auto_laden_ist_an() ? "an" : (auto_lastschutz ? "schutz" : "aus"),
				auto_benoetigte_ladeleistung_in_w,
				aktuelle_auto_ladeleistung_in_w,
				_gib_genutzte_auto_ladeleistung_in_w(),
				wasser_relay_ist_an ? "an" : (wasser_lastschutz ? "schutz" : "aus")
			);
			file_writer.write_formated(
				",vb7,%s,%s,%d,%d,%d,%s,%s,%s",
				(roller_ladestatus == Local::Model::Verbraucher::Ladestatus::force ? "force" : "solar"),
				roller_laden_ist_an() ? "an" : (roller_lastschutz ? "schutz" : "aus"),
				roller_benoetigte_ladeleistung_in_w,
				aktuelle_roller_ladeleistung_in_w,
				_gib_genutzte_roller_ladeleistung_in_w(),
				heizung_relay_ist_an ? "an" : (heizung_lastschutz ? "schutz" : "aus"),
				ladeverhalten_wintermodus ? "an" : "aus"
			);
		}
	};
}
