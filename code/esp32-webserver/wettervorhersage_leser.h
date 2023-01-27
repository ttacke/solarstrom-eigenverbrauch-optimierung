#pragma once
#include <ArduinoJson.h>
#include "base_leser.h"

namespace Local {
	class WettervorhersageLeser: public BaseLeser {

	using BaseLeser::BaseLeser;

	public:
		void daten_holen_und_einsetzen(Local::ElektroAnlage& elektroanlage) {
			// TODO Auf dem String direkt arbeiten und via Regex die Werte holen
			// JSON ist hier zu gross, weil es doppelten Speicher belegt
			// das auch überall tun
			// TODO API-Key und LocationID einsetzen: als Char) und das fix als URL speichern
			// damit das nicht jedes mal passiert

			// TODO die Uhrzeit bei einem Fehler in der UI ist falsch?!?


//			DynamicJsonDocument d = string_to_json(
//				web_client.get("https://dataservice.accuweather.com/forecasts/v1/hourly/1hour/XXX?apikey=XXX&language=de-de&details=true&metric=true")
//			);
//			Serial.println(round((float) d[0]["SolarIrradiance"]["Value"]));
// nur 50x pro tag. D.h. die Zeitgeschichte muss gelöst sein, ansonsten speichert das Ding seinen Wert auf der SD zwischen???
// das muss woanders passieren. Das gehört hier nicht rein mit der SD.
// Besser: Die Solardaten sind ein einzelnes Objekt, was immer am ende geschrieben wird. Aber nur, wenn es änderungen gab.
//   ansonsten wird es beim start aus der SD gelesen
//   kann man auch auf den Flash-Speicher schreiben? (aber: will man das? SD ist eh da)


//			Serial.println((char*) d[0]["SolarIrradiance"]["Unit"]);
		}
	};
}
