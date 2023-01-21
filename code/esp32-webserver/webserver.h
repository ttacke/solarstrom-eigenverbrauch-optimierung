#pragma once
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

class Webserver {
protected:
	ESP8266WebServer server;
  String* _erzeuge_not_found_seite() {
    return new String[3]{"404", "text/html", "<h1>Not Found</h1>"};
    
  }
public:
	Webserver(int port) {
		ESP8266WebServer server(port);
	}

  void add_page(String path, auto func) {
    server.on(path, [&]() {
      String* result = func();
      server.send(result[0].toInt(), result[1], result[2]);
    });
  }
  
	void start() {
    server.onNotFound([&]() {
      String* result = _erzeuge_not_found_seite();
      server.send(result[0].toInt(), result[1], result[2]);
    });
    if (MDNS.begin("esp8266")) {
      Serial.println("MDNS responder started");
    }
	  server.begin();
	  Serial.println("HTTP server started");
	}
	void watch_for_client() {
		server.handleClient();
		MDNS.update();
	}
};
