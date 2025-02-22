#pragma once

namespace Local {
	class Config {
	public:
		const int log_baud = 9600;

		const char* network_connection_check_host = "192.168.0.1";
		const char* network_connection_check_path = "/unknownpath";
		const char* wlan_ssid = "[WLAN-SSID]";
		const char* wlan_pwd = "[WLAN-PASSWORT]";

		const char* target_host = "[HOST]";
		const int target_port = 80;
		const char* target_path = "/set_heat_difference?val=";
	};
}
