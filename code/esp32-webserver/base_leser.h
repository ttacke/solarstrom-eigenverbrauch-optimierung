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
		MatchState match_state;
		char capture[32];

		String _finde(char* regex, String content) {
			char c_content[content.length() + 1];
			for(int i = 0; i < content.length(); i++) {
				c_content[i] = content.charAt(i);
			}
			c_content[content.length() + 1] = '\0';// Null am ende explizit setzen

			match_state.Target(c_content);
			char result = match_state.Match(regex);
			if(result > 0) {
				// content.substring(match_state.MatchStart, match_state.MatchStart + match_state.MatchLength);
				match_state.GetCapture(capture, 0);
				return (String) capture;
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
