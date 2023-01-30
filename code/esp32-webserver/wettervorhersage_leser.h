#pragma once
#include <ArduinoJson.h>
#include "base_leser.h"
#include "persistenz.h"
#include "wetter.h"
#include <Regexp.h>

namespace Local {
	class WettervorhersageLeser: public BaseLeser {

	using BaseLeser::BaseLeser;
	protected:
		const char* filename = "wetter_stundenvorhersage.json";
		MatchState ms;

	public:
		// TODO das geh auch in einem Schritt, Response blockweise lesen und scannen, und nur
		// in Wettervorhersage speichern
		void daten_holen_und_persistieren(Local::Persistenz& persistenz) {
			persistenz.write2file(
				(char*) filename,
				web_client.get(cfg.wetter_stundenvorhersage_url)
			);
			Serial.println("Wetterdaten geschrieben");
		}

		String _finde(char* regex, String content) {
			char c_content[content.length() + 1];
			for(int i = 0; i < content.length(); i++) {
				c_content[i] = content.charAt(i);
			}

			ms.Target(c_content);
			char result = ms.Match(regex);
			if(result > 0) {
				// content.substring(ms.MatchStart, ms.MatchStart + ms.MatchLength);
				char cap[16];
				ms.GetCapture(cap, 0);
				return (String) cap;
			}
			return "";
		}

		void persistierte_daten_einsetzen(Local::Persistenz& persistenz, Local::Wetter& wetter) {
			int offset = 0;
			String old_block = "";
			int zeitpunkt_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			int solarstrahlung_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			int wolkendichte_liste[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
			int i = 0;
			while(true) {
				String block = persistenz.read_file_content_block((char*) filename, offset);

				int zeitpunkt = _finde(
					(char*) "\"EpochDateTime\":([0-9]+)[,}]",
					old_block + block
				).toInt();
				if(zeitpunkt != 0 && zeitpunkt_liste[i] != zeitpunkt) {
					if(zeitpunkt_liste[0] != 0) {
						i++;
					}
					zeitpunkt_liste[i] = zeitpunkt;
				}

				// Nur den Ganzzahlwert, Nachkommastellen sind irrelevant
				int solarstrahlung = _finde(
					(char*) "\"SolarIrradiance\":{[^}]*\"Value\":([0-9]+).[0-9][,}]",
					old_block + block
				).toInt();
				if(solarstrahlung != 0 && solarstrahlung_liste[i] != solarstrahlung) {
					solarstrahlung_liste[i] = solarstrahlung;
				}

				int wolkendichte = _finde(
					(char*) "\"CloudCover\":([0-9]+)[,}]",
					old_block + block
				).toInt();
				if(wolkendichte != 0 && wolkendichte_liste[i] != wolkendichte) {
					wolkendichte_liste[i] = wolkendichte;
				}

				old_block = block;
				offset += block.length();
				if(block.length() == 0) {
					break;
				}
			}
			// TODO ggf mal pruefen, ob der ersteZeitstempel in der vergangenheit, drer 2, in der Zukunf liegt
			// aber das knallt immer on um bis zum abholen des nächsten :/
			Serial.println(zeitpunkt_liste[0]);// TODO
			if(zeitpunkt_liste[0] > 0) {
				wetter.daten_vorhanden = true;
				for(int i = 0; i < 12; i++) {
					wetter.setze_stundenvorhersage_solarstrahlung(i, solarstrahlung_liste[i]);
					wetter.setze_stundenvorhersage_wolkendichte(i, wolkendichte_liste[i]);
				}
			} else {
				wetter.daten_vorhanden = false;
			}
		}
	};
}
