#pragma once
#include <ArduinoJson.h>

// TODO DEPRECATED
namespace Local {
	class ContentConverter {
	public:
		ContentConverter() {
		}
		 DynamicJsonDocument string_to_json(String content) {
		  DynamicJsonDocument doc(content.length() * 2);
		  DeserializationError error = deserializeJson(doc, content);
		  if (error) {
			Serial.print(F("deserializeJson() failed with code "));
			Serial.println(error.c_str());
			return doc;
		  }
		  return doc;
		}
		String formatiere_zahl(bool force_prefix, int value) {
		  char str[50];
		  if(value > 999 || value < -999) {
			sprintf(str, (force_prefix ? "%+.3f" : "%.3f"), (float) value / 1000);
		  } else {
			sprintf(str, (force_prefix ? "%+d" : "%d"), value);
		  }
		  return str;
		}
		String formatiere_stromstaerke(int value) {
		  char str[50];
		  sprintf(str, ("%.1f"), (float) value / 1000);
		  return str;
		}
	};
}
