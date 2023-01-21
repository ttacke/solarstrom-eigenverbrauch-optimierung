#pragma once
#include <WiFiClient.h>

class Wlan {
public:
	WiFiClient client;
  Wlan(const char* ssid, const char* pwd) {
    
  }
};
