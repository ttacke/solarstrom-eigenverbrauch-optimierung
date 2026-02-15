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
			return _gib_listen_maximum(Local::SemipersistentData::auto_ladeleistung_log_in_w, 5);
		}

		int _gib_genutzte_roller_ladeleistung_in_w() {
			return _gib_listen_maximum(Local::SemipersistentData::roller_ladeleistung_log_in_w, 5);
		}

	public:
		enum class Ladestatus {force, solar};

		bool heiz_verdichter_relay_ist_an = false;
		int heiz_verdichter_laeuft_seit = 0;
		int heiz_verdichter_aus_seit = 0;

		bool wasser_begleitheizung_relay_is_an = false;
		bool wasser_begleitheizung_lastschutz = false;
		int wasser_begleitheizung_aktuelle_leistung_in_w = 0;

		bool wasser_relay_ist_an = false;
		bool wasser_lastschutz = false;
		int wasser_relay_zustand_seit = 0;

		int wasser_wp_aktuelle_leistung_in_w = 0;

		bool heizung_relay_ist_an = false;
		bool heizung_ist_an = false;
		bool heizung_lastschutz = false;
		int heizung_relay_zustand_seit = 0;

		Ladestatus auto_ladestatus = Local::Model::Verbraucher::Ladestatus::solar;
		int auto_ladestatus_seit = 0;
		int auto_benoetigte_ladeleistung_in_w = 0;
		int aktuelle_auto_ladeleistung_in_w = 0;
		bool auto_relay_ist_an = false;
		bool auto_lastschutz = false;
		int auto_relay_zustand_seit = 0;

		Ladestatus roller_ladestatus = Local::Model::Verbraucher::Ladestatus::solar;
		int roller_ladestatus_seit = 0;
		int roller_benoetigte_ladeleistung_in_w = 0;
		int aktuelle_roller_ladeleistung_in_w = 0;
		bool roller_relay_ist_an = false;
		bool roller_lastschutz = false;
		int roller_relay_zustand_seit = 0;

		int aktueller_verbrauch_in_w = 0;
		int aktuelle_erzeugung_in_w = 0;
		int aktueller_akku_ladenstand_in_promille = 0;
		int akku_ladestandsvorhersage_in_promille[48];
		int solarerzeugung_in_w = 0;
		int zeitpunkt_sonnenuntergang = 0;
		bool ersatzstrom_ist_aktiv = false;
		bool ladeverhalten_wintermodus = false;
		int netzbezug_in_w = 0;

		float waermepumpen_zuluft_temperatur = 0;
		float waermepumpen_abluft_temperatur = 0;
		int heizungs_temperatur_differenz = 0;
		float heizungs_temperatur_differenz_in_grad = 0;
		bool heizstabbetrieb_ist_erlaubt = false;

		void setze_heizungs_temperatur_differenz(
			int diff, float heizungs_temperatur_differenz_umrechnungsfaktor, int heizungs_temperatur_differenz_nullpunkt
		) {
			heizungs_temperatur_differenz = diff - heizungs_temperatur_differenz_nullpunkt;
			if(heizungs_temperatur_differenz < 0) {
				heizungs_temperatur_differenz = 0;
			}
			heizungs_temperatur_differenz_in_grad = heizungs_temperatur_differenz / heizungs_temperatur_differenz_umrechnungsfaktor;
		}

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
			if(aktueller_akku_ladenstand_in_promille <= ladestand_in_promille) {
				return true;
			}
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
				erzeugung += Local::SemipersistentData::erzeugung_log_in_w[i];
			}

			int max_verbrauch = aktueller_verbrauch_in_w;
			for(int i = 0; i < 5; i++) {
				if(max_verbrauch < Local::SemipersistentData::verbrauch_log_in_w[i]) {
					max_verbrauch = Local::SemipersistentData::verbrauch_log_in_w[i];
				}
			}
			return round((erzeugung / 31) - max_verbrauch);
		}

		void write_log_data(Local::Service::FileWriter& file_writer) {
			file_writer.write_formated(
				"va4,%s,%s,%s,%d",
				(auto_ladestatus == Local::Model::Verbraucher::Ladestatus::force ? "force" : "solar"),
				auto_laden_ist_an() ? "on" : "off",
				auto_lastschutz ? "on" : "off",
				auto_benoetigte_ladeleistung_in_w
			);
			file_writer.write_formated(
				",%d,%d,%s,%s",
				aktuelle_auto_ladeleistung_in_w,
				_gib_genutzte_auto_ladeleistung_in_w(),
				wasser_relay_ist_an ? "on" : "off",
				wasser_lastschutz ? "on" : "off"
			);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
			file_writer.write_formated(
				",vb8,%s,%s,%s,%d",
				(roller_ladestatus == Local::Model::Verbraucher::Ladestatus::force ? "force" : "solar"),
				roller_laden_ist_an() ? "on" : "off",
				roller_lastschutz ? "on" : "off",
				roller_benoetigte_ladeleistung_in_w
			);
			file_writer.write_formated(
				",%d,%d,%s,%s,%s",
				aktuelle_roller_ladeleistung_in_w,
				_gib_genutzte_roller_ladeleistung_in_w(),
				heizung_relay_ist_an ? "on" : "off",
				heizung_lastschutz ? "on" : "off",
				ladeverhalten_wintermodus ? "on" : "off"
			);
			yield();// ESP-Controller zeit fuer interne Dinge (Wlan z.B.) geben
			file_writer.write_formated(
				",vc2,%s,%d,%s,%d,%d",
				"off",// DEPRECATED Heiz-WP-Luftvorwaermer
				0,// DEPRECATED Heiz-WP-Luftvorwaermer Leistung in W
				wasser_begleitheizung_relay_is_an ? "on" : "off",
				wasser_begleitheizung_aktuelle_leistung_in_w,
				wasser_wp_aktuelle_leistung_in_w
			);
		}
	};
}
