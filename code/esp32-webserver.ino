// Webserver fuer Anzeige auf einem KindleReader
// CSS/JS ist auf den dortigen "ExperimentalBrowser" angepasst

// Hinweis: auf dem Kindle den Bildschirm dauerhaft einschalten: "~ds" im Suchfeld eingeben
// https://ebooks.stackexchange.com/questions/152/what-commands-can-be-given-in-the-kindles-search-box

// Es ist "Selbstheilend": Wenn der ESP nicht antwortet, wird die anzeige ausgegraut und beim nächsten Abruf
// einfach erneut versucht

// CONFIG
const char* wechselrichter_data_url = "http://192.168.0.14/status/powerflow";// Fronius Gen24 8kW
const char* smartmeter_data_url = "http://192.168.0.14/components/PowerMeter/readable";// FromiusSmartMeter via Fronius Gen24 8kW
const char* smartmeter_id = "16711680";// Die Nummer wird in der data_url geliefert und ist fix, taucht aber sonst nirgends auf
const float speicher_groesse = 7650;// in Wh
const char* wlan_ssid = "[BITTE-EINFUEGEN]";
const char* wlan_pwd = "[BITTE-EINFUEGEN]";
const char* refresh_display_interval = "5 * 60";// In welchen Zeitabstaenden (in Sekunden) soll die Anzeige aktualisiert werden
const int log_baud = 9600;
// END CONFIG

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

ESP8266WebServer server(80);

void setup(void) {
  Serial.begin(log_baud);
  _connect_to_wlan(wlan_ssid, wlan_pwd);
  _start_webserver();
}

void loop(void) {
  _webserber_watch_for_client();
}

// ###################################################################################

void _webserber_watch_for_client() {
  server.handleClient();
  MDNS.update();
}

void _start_webserver() {
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  server.on("/", _erzeuge_website);
  server.onNotFound(_erzeuge_not_found_seite);
  server.begin();
  Serial.println("HTTP server started");
}

void _connect_to_wlan(char* wlan_ssid, char* wlan_password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wlan_ssid, wlan_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WLAN Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

DynamicJsonDocument _string_to_json(String content, int obj_size) {
  DynamicJsonDocument doc(obj_size);
  auto error = deserializeJson(doc, content);
  if (error) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(error.c_str());
    return doc;
  }
  return doc;
}

String _fetch_http_content(char const* url) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK) {
          const String payload = http.getString();
          http.end();
          return payload;
      } else {
        Serial.printf("[HTTP] GET failed, code: %d\n", httpCode);
      }
    } else {
      Serial.printf("[HTTP] GET failed, error: ");
      Serial.printf(http.errorToString(httpCode).c_str());
    }
    http.end();
    return "ERROR";
}

String _format_power(bool force_prefix, int value) {
  char str[50];
  if(value > 999 || value < -999) {
    sprintf(str, (force_prefix ? "%+.3f" : "%.3f"), (float) value / 1000);
  } else {
    sprintf(str, (force_prefix ? "%+d" : "%d"), value);
  }
  return str;
}

String _format_current(int value) {
  char str[50];
  sprintf(str, ("%.1f"), (float) value / 1000);
  return str;
}

DynamicJsonDocument _hole_maximalen_strom_und_phase() {
  const String smartmeter_content = _fetch_http_content(smartmeter_data_url);
  DynamicJsonDocument s_data = _string_to_json(smartmeter_content, 8096);
  return _hole_maximalen_strom_und_phase_aus_json(s_data["Body"]["Data"][smartmeter_id]["channels"]);
}

DynamicJsonDocument _hole_wechselrichter_daten() {
  const String wechselrichter_content = _fetch_http_content(wechselrichter_data_url);
  DynamicJsonDocument data = _string_to_json(wechselrichter_content, 2048);

  DynamicJsonDocument result(512);
  result["SOC"] = data["inverters"][0]["SOC"];
  result["BatMode"] = data["inverters"][0]["BatMode"];
  result["P_Akku"] = data["site"]["P_Akku"];
  result["P_Load"] = data["site"]["P_Load"];
  result["P_Grid"] = data["site"]["P_Grid"];
  result["P_PV"] = data["site"]["P_PV"];
  return result;
}

DynamicJsonDocument _hole_daten() {
  DynamicJsonDocument w_data1 = _hole_wechselrichter_daten();
  DynamicJsonDocument max_i1 = _hole_maximalen_strom_und_phase();

  delay(3000);
  DynamicJsonDocument w_data2 = _hole_wechselrichter_daten();
  DynamicJsonDocument max_i2 = _hole_maximalen_strom_und_phase();

  delay(3000);
  DynamicJsonDocument w_data3 = _hole_wechselrichter_daten();
  DynamicJsonDocument max_i3 = _hole_maximalen_strom_und_phase();

  DynamicJsonDocument max_i = max_i1;
  if((int) max_i2["MAX_I"] > (int) max_i["MAX_I"]) {
    max_i = max_i2;
  }
  if((int) max_i3["MAX_I"] > (int) max_i["MAX_I"]) {
    max_i = max_i3;
  }
  w_data1["MAX_I"] = (int) max_i["MAX_I"];
  w_data1["MAX_I_PHASE"] = max_i["MAX_I_PHASE"];

  w_data1["SOC"] = ((float) w_data1["SOC"] + (float) w_data2["SOC"] + (float) w_data3["SOC"]) / 3;
  w_data1["BatMode"] = ((float) w_data1["BatMode"] + (float) w_data2["BatMode"] + (float) w_data3["BatMode"]) / 3;
  w_data1["P_Akku"] = ((float) w_data1["P_Akku"] + (float) w_data2["P_Akku"] + (float) w_data3["P_Akku"]) / 3;

  w_data1["P_Load"] = ((float) w_data1["P_Load"] + (float) w_data2["P_Load"] + (float) w_data3["P_Load"]) / 3;
  w_data1["P_Grid"] = ((float) w_data1["P_Grid"] + (float) w_data2["P_Grid"] + (float) w_data3["P_Grid"]) / 3;
  w_data1["P_PV"] = ((float) w_data1["P_PV"] + (float) w_data2["P_PV"] + (float) w_data3["P_PV"]) / 3;

  return w_data1;
}

DynamicJsonDocument _hole_maximalen_strom_und_phase_aus_json(DynamicJsonDocument data) {
  float i1 = (float) data["SMARTMETER_CURRENT_01_F64"];
  float i2 = (float) data["SMARTMETER_CURRENT_02_F64"];
  float i3 = (float) data["SMARTMETER_CURRENT_03_F64"];
  if(i1 < 0) {
    i1 = i1 * -1;
  }
  if(i2 < 0) {
    i2 = i2 * -1;
  }
  if(i3 < 0) {
    i3 = i3 * -1;
  }
  DynamicJsonDocument result(128);
  if(i3 > i2 && i3 > i1) {
    result["MAX_I"] = round(i3 * 1000);
    result["MAX_I_PHASE"] = 3;
  } else if(i2 > i1) {
    result["MAX_I"] = round(i2 * 1000);
    result["MAX_I_PHASE"] = 2;
  } else {
    result["MAX_I"] = round(i1 * 1000);
    result["MAX_I_PHASE"] = 1;
  }
  return result;
}

void _erzeuge_website() {
  DynamicJsonDocument data = _hole_daten();

  const int speicher_stand = (float) data["SOC"];
  const int speicher_modus = (int) data["BatMode"];
  const int akku = round((float) data["P_Akku"]);

  const int load = round((float) data["P_Load"]) * -1;
  const int grid = round((float) data["P_Grid"]);
  const int solar = round((float) data["P_PV"]);
  const int max_i = (int) data["MAX_I"];
  const String max_i_phase = (String) data["MAX_I_PHASE"];
  if(!load && !grid && !solar && !max_i && !max_i_phase) {
    server.send(503, "text/plain", "");
    return;
  }

  const String speicher_ladung = _format_power(false, speicher_groesse / 100 * speicher_stand);
  String speicher_status = "";
  if(speicher_modus != 1 && speicher_modus != 14) {
    speicher_status = "speicher_aus";
  }
/*
http://192.168.0.106/main-es2015.57cf3de98e3c435ccd68.js
BatMode
case 0: "disabled"
-- 1: aktiv
case 2: "serviceMode"
case 3: "chargeBoost"
case 4: "nearlyDepleted"
case 5: "suspended"
case 6: "calibration"
case 8: "depletedRecovery"
case 12: "startup"
case 13: "stoppedTemperature"
case 14: "maxSocReached"
*/
  String html = "<html><head><title>Sonnenrech 19</title>"
    "<script type=\"text/javascript\">"
    "var anzahl_fehler = 0;\n"
    "function setze_lokale_daten() {\n"
    "  var max_i = document.getElementById('max_i_value').innerHTML;\n"
    "  if(!document.max_i || document.max_i < max_i) {\n"
    "   document.max_i = max_i;\n"
    "   document.max_i_phase = document.getElementById('max_i_phase').innerHTML;\n"
    "   var now = new Date();\n"
    "   document.max_i_uhrzeit = now.getHours() + ':' + (now.getMinutes() < 10 ? '0' : '') + now.getMinutes();\n"
    "  }\n"
    "  document.getElementById('marker').innerHTML = document.max_i_uhrzeit;\n"
    "  document.getElementById('max_i').innerHTML = 'max I=' + document.max_i + 'A&nbsp(' + document.max_i_phase + ')&nbsp;';\n"
    "}\n"
    "function reload() {\n"
    "  var xhr = new XMLHttpRequest();\n"
    "  xhr.onreadystatechange = function(args) {\n"
    "    if(this.readyState == this.DONE) {\n"
    "      if(this.status == 200) {\n"
    "        anzahl_fehler = 0;\n"
    "        document.getElementsByTagName('html')[0].innerHTML = this.responseText;\n"
    "        setze_lokale_daten();\n"
    "        document.getElementsByTagName('body')[0].className = '';\n"
    "      } else {\n"
    "        anzahl_fehler++;\n"
    "        document.getElementById('max_i').innerHTML = '';\n"
    "        document.getElementById('fehler').innerHTML = ' ' + anzahl_fehler + ' FEHLER!';\n"
    "        document.getElementsByTagName('body')[0].className = 'hat_fehler';\n"
    "      }\n"
    "    }\n"
    "  };\n"
    "  xhr.open('GET', '/', true);\n"
    "  xhr.timeout = 30 * 1000;\n"
    "  xhr.send();\n"
    "}\n"
    "setInterval(reload, " + refresh_display_interval + " * 1000);\n"
    "</script>"
    "<style>"
    "body{padding:0.5rem;padding-top:0.5rem;}"//transform:rotate(180deg); geht im Kindle nicht
    "*{font-family:sans-serif;font-weight:bold;color:#000;}"
    "td{font-size:5rem;line-height:1.1;}"
    "table{width:100%;}"
    "tr.last td {border-top:5px solid #000;}"
    ".zahl{text-align:right;padding-left:1rem;}"
    ".label{font-size:2rem;}"
    ".einheit{padding-left:0.3rem;}"
    ".markerline td{text-align:center;font-size:2.5rem;}"
    ".markerline td span{color:#999;}"
    ".markerline td #max_i{}"
    ".markerline td #marker{}"
    ".speicher_aus td{color:#999;}"
    ".hat_fehler *{color:#999;}"
    ".hat_fehler .markerline td span{color:#000;text-decoration:underline;}"
    "</style>"
    "</head><body onclick=\"reload();\" onload=\"setze_lokale_daten();\">"
    "<div style=\"display:none\"><span id=max_i_value>" + _format_current(max_i) + "</span><span id=max_i_phase>" + max_i_phase + "</span></div>"
    "<table cellspacing=0>"
    "<tr><td class=label>Solar</td><td class=zahl>" + _format_power(false, solar) + "</td><td class=\"label einheit\">W</span></td></tr>"
    "<tr><td class=label>+Akku<br/></td><td class=zahl>" + _format_power(true, akku) + "</td><td class=\"label einheit\">W</span></td></tr>"
    "<tr><td class=label>+Netz</td><td class=zahl>" + _format_power(true, grid) + "</td><td class=\"label einheit\">W</span></td></tr>"
    "<tr class=last><td class=label>=Last</td><td class=zahl>" +  _format_power(false, load) + "</td><td class=\"label einheit\">W</span></td></tr>"
    "<tr class=markerline><td colspan=3><span id=max_i></span><span id=marker>starte...</span><span id=fehler></span></td></tr>"
    "<tr class=\"speicher " + speicher_status + "\"><td class=label>Speicher</td><td class=zahl>" + speicher_ladung + "</td><td class=\"label einheit\">Wh</span></td></tr>"
    "</table>"
    "</body></html>";

  server.send(200, "text/html", html);
}

void _erzeuge_not_found_seite() {
  server.send(404, "text/html", "<h1>Not Found</h1>");
}
