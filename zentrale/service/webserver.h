#pragma once
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

namespace Local::Service {
	class Webserver {
	public:
		ESP8266WebServer server;// ist via unique_ptr gesichert, es kann nur einen Pointer geben!

		Webserver(int port): server(ESP8266WebServer(port)) {
		}

		template<typename F>
		void add_http_get_handler(const char* path, F && func) {
			server.on(path, HTTP_GET, func);
		}

		template<typename F>
		void add_http_post_fileupload_handler(const char* path, F && func) {
			server.on(path, HTTP_POST, [&](){ server.send(200); }, func);
		}

		void start() {
			server.onNotFound([&]() {
				server.send(404, "text/html", "<h1>Not Found</h1>");
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
