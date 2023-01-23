#pragma once
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "http_response.h"

namespace Local {
	class Webserver {
	protected:
		ESP8266WebServer server;
		Local::HTTPResponse _erzeuge_not_found_seite() {
			return Local::HTTPResponse(404, "text/html", "<h1>Not Found</h1>");
		}
	public:
		Webserver(int port) {
			ESP8266WebServer server(port);
		}

	  template<typename F>
	  void add_page(String path, F && func) {
		server.on(path, [&]() {
		  Local::HTTPResponse response = func();
		  Serial.println("Antwort");
		  Serial.println(response.return_code);
		  Serial.println(response.content.length());
		  server.send(response.return_code, response.content_type, response.content);
		});
	  }

		void start() {
		server.onNotFound([&]() {
		  Local::HTTPResponse response = _erzeuge_not_found_seite();
		  server.send(response.return_code, response.content_type, response.content);
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
}
