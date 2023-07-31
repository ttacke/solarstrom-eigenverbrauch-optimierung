#pragma once

namespace Local {
	class Config {
	public:
		const int log_baud = 9600;

		const char* network_connection_check_host = "192.168.2.1";
		const char* network_connection_check_path = "/css/login.css";
		const char* network_connection_check_content = "body";
		const char* wlan_ssid = "[WLAN-SSID]";
		const char* wlan_pwd = "[WLAN-PASSWORT]";

		const int webserver_port = 80;
	};
}
