// WLAN-On/Off nicht zu oft, denn das wird immer ins Flash geschrieben was die Haltbarkeit verringert
#include "config.h"
#include "wlan.h"
#include "web_reader.h"
#include <stdlib.h>

Local::Config cfg;
Local::Wlan wlan(cfg.wlan_ssid, cfg.wlan_pwd);
Local::WebReader web_reader(wlan.client);
int sensor_values_pointer = 0;
int sensor_values[300]; // Anzahl der Sekunden der Messdauer
int sensor_values_length = 300; // Anzahl der Sekunden der Messdauer
int wlan_pause = 10; // Anzahl der Sekunden, die nach WLAN Trennung gewartet wird
char buffer[64];

void _add_sensor_value(int new_val) {
	sensor_values[sensor_values_pointer] = new_val;
	sensor_values_pointer++;
}
bool _sensor_values_are_filled() {
	if(sensor_values_pointer >= sensor_values_length) {
		return true;
	}
	return false;
}
int _compare_int_values(const void* a, const void* b) {
	int int_a = * ( (int*) a );
	int int_b = * ( (int*) b );
	if ( int_a == int_b ) return 0;
	else if ( int_a < int_b ) return -1;
	else return 1;
}
int _get_sensor_median_and_reset_values() {
	sensor_values_pointer = 0;
	qsort(sensor_values, sensor_values_length, sizeof(int), _compare_int_values);
	return sensor_values[(int) round((float) sensor_values_length / 2)];
}
void _post_sensor_value(int value) {
	sprintf(buffer, "%s%d", cfg.target_path, value);
	Serial.println("Send value: " + (String) buffer);
	web_reader.send_http_get_request(
		cfg.target_host,
		cfg.target_port,
		buffer,
		web_reader.default_timeout_in_hundertstel_s
	);
}
void setup(void) {
	Serial.begin(cfg.log_baud);
	for(int i = 0; i < 5; i++) {
		Serial.print('o');
		delay(500);
	}
	Serial.println("\nStart strom-eigenverbrauch-optimierung Temperatur-Differenz-Messer");
	Serial.printf("Free stack: %u heap: %u\n", ESP.getFreeContStack(), ESP.getFreeHeap());
}
void loop(void) {
	_add_sensor_value(analogRead(A0));
	if(_sensor_values_are_filled()) {
		int value = _get_sensor_median_and_reset_values();
		wlan.connect();
		_post_sensor_value(value);
		wlan.disconnect();
		delay(wlan_pause * 1000);
	} else {
		delay(1000);
	}
}
