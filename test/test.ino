#include "config.h"
#include "wlan.h"
#include "web_reader.h"

Local::Config cfg;
Local::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
Local::WebReader web_reader(wlan.client);

void setup(void) {
	Serial.begin(cfg.log_baud);
	for(int i = 0; i < 5; i++) {
		Serial.print('o');
		delay(500);
	}
	Serial.println("\nStart Test");
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
}

bool _run_test() {
	if(!wlan.is_connected()) {
		Serial.println("Connect WLAN");
		wlan.connect();
		if(!wlan.is_connected()) {
			return false;
		}
	}
	/*

	 */
	if(!web_reader.send_http_get_request(
		//Closed
		//(char*)"192.168.0.39",80,(char*)"/relay"
		//(char*)"192.168.0.39",80,(char*)"/rpc/Switch.GetStatus?id=0"
		//(char*)"192.168.0.36",80,(char*)"/status"
		//CHUNK
		(char*)"api.open-meteo.com",80,(char*)"/v1/forecast?latitude=50.00&longitude=7.95&daily=sunrise,sunset,shortwave_radiation_sum&hourly=global_tilted_irradiance_instant&timezone=Europe/Berlin&tilt=35&azimuth=30&timeformat=unixtime&forecast_hours=12"
		//(char*)"192.168.0.14",80,(char*)"/solar_api/v1/GetMeterRealtimeData.cgi"
		//(char*)"192.168.0.14",80,(char*)"/solar_api/v1/GetPowerFlowRealtimeData.fcgi"
		//(char*)"192.168.0.14",80,(char*)"/components/inverter/readable"
	)) {
		Serial.println("Webreader-Read: Failed");
		return false;
	}
	while(web_reader.read_next_block_to_buffer()) {
		if(web_reader.find_in_buffer((char*) "\"DONTFINDME\":")) {
			Serial.println("Webreader-Find: Found!");
			return true;
		}
	}
	Serial.println("Webreader-Find: Finished");
	return true;
}

void loop(void) {
	Serial.println("Run test:");
	if(_run_test()) {
		Serial.println("End test: Success");
	} else {
		Serial.println("End test: Error");
	}
	delay(6000000);
}
