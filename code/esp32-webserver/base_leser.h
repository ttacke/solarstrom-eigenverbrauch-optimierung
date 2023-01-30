#pragma once
#include "web_client.h"
#include "config.h"
//#include <ArduinoJson.h>
#include <Regexp.h>

namespace Local {
	class BaseLeser {

	protected:
		Local::WebClient web_client;
		Local::Config cfg;
		MatchState ms;

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

//		DynamicJsonDocument string_to_json(String content) {
//		  DynamicJsonDocument doc(round(content.length() * 2));
//		  DeserializationError error = deserializeJson(doc, content);
//		  if (error) {
//			Serial.print(F("deserializeJson() failed with code "));
//			Serial.println(error.c_str());
//			return doc;
//		  }
//		  return doc;
//		}
	public:
		explicit BaseLeser(Local::Config& cfg, Local::WebClient& web_client): cfg(cfg), web_client(web_client) {
		}
	};
}
