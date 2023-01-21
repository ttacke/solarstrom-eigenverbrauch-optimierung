#pragma once
#include <ArduinoJson.h>

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
};
