#include "config.h"
#include "wlan.h"
#include "webserver.h"
#include "web_client.h"
#include "content_converter.h"
#include "elektro_anlage.h"
#include "wechselrichter_leser.h"
#include <ArduinoJson.h>
#include "http_response.h"

Local::Wlan wlan(Local::Config::wlan_ssid, Local::Config::wlan_pwd);
Local::Webserver webserver(80);
Local::WebClient web_client(wlan.client);
/* TODO
- Alle eigenen Klassen in den Local Namespace
- Die Batterierechnung ist kaputt?!? Wieso?
BaseLeser erstellen mit JsonReader und Webclient
Davon erben
SmartmeterLeser erstellen und Stromstärken auslesen
Display erstellen und HTML dort ausgeben
Berechnungen erfolgen in ElekrtoAnlage
Alle Leser im Kopf erstellen und dann an passender Stelle nutzen
Config als Objekt erstelle und nutzen.
Wechselrichter-IP in der Config, die URL im Leser (die ist eh fix an die Struktur gebunden)

5,5 Zeilen hat das Display in ordentlicher größe
=========
Überschuss: 1.500 W <- enthält auch das Laden der Batterie
SolarBatt:     72 %
Auto Laden: 54% [nur Überschuss] [normal] <- Ü ist inaktiv, wenn nicht sinnvoll
Vorhersage: 8:15 - 17:02 [Überschuss wird erwartet] <- woher kriegen wir so einen Wert?
Last [Netz 2000W + Batterieladestrom (wenn geladen, nicht, wenn entladen)] <-- noch unklar
=========
*/
// DEPRECATED
Local::ContentConverter content_converter;

void setup(void) {
	// TODO Config meistens als Objekt nutzen
  Serial.begin(Local::Config::log_baud);
  Serial.println("\nSetup ESP");
  wlan.connect();
  Local::Config cfg;
  Local::ElektroAnlage elektroanlage;
  webserver.add_page("/", [&]() {
  	// TODO leser hier schon setzen?
    return _erzeuge_website(cfg, elektroanlage);
  });
  webserver.start();
}

void loop(void) {
  webserver.watch_for_client();
}

// ###################################################################################


DynamicJsonDocument _hole_maximalen_strom_und_phase() {
  const String smartmeter_content = web_client.get(Local::Config::smartmeter_data_url);
  Serial.println("strom");
  DynamicJsonDocument s_data = content_converter.string_to_json(smartmeter_content);


  // TODO hier klappt der Zugriff auf einmal nicht mehr. Nur, weil Inline? Warum muss an dieser Stelle dieser Umweg passieren?
  DynamicJsonDocument b(1024);
  b = s_data["Body"]["Data"][Local::Config::smartmeter_id]["channels"];
  return _hole_maximalen_strom_und_phase_aus_json(b);
}

DynamicJsonDocument _hole_wechselrichter_daten(Local::Config config, Local::ElektroAnlage elektroanlage) {
	// TODO dennach ganz außen setzen
  Local::WechselrichterLeser leser(config, web_client);
  leser.daten_holen_und_einsetzen(elektroanlage);

	// DEPRECATED
	  DynamicJsonDocument result(512);
	  result["SOC"] = (String) elektroanlage.solarakku_ladestand_prozent;
	  result["BatMode"] = (int) (elektroanlage.solarakku_ist_an ? 1 : 0);
	  result["P_Akku"] = (String) elektroanlage.solarakku_wh;
	  result["P_Load"] = (String) elektroanlage.verbraucher_wh;
	  result["P_Grid"] = (String) elektroanlage.netz_wh;
	  result["P_PV"] = (String) elektroanlage.solar_wh;
	  return result;
}

DynamicJsonDocument _hole_daten(Local::Config config, Local::ElektroAnlage elektroanlage) {
  DynamicJsonDocument w_data1 = _hole_wechselrichter_daten(config, elektroanlage);
  DynamicJsonDocument max_i1 = _hole_maximalen_strom_und_phase();

  w_data1["MAX_I"] = (int) max_i1["MAX_I"];
  w_data1["MAX_I_PHASE"] = max_i1["MAX_I_PHASE"];

  w_data1["SOC"] = (float) w_data1["SOC"];
  w_data1["BatMode"] = (int) w_data1["BatMode"];
  w_data1["P_Akku"] = (float) w_data1["P_Akku"];

  w_data1["P_Load"] = (float) w_data1["P_Load"];
  w_data1["P_Grid"] = (float) w_data1["P_Grid"];
  w_data1["P_PV"] = (float) w_data1["P_PV"];

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

Local::HTTPResponse _erzeuge_website(Local::Config cfg, Local::ElektroAnlage elektroanlage) {
  DynamicJsonDocument data = _hole_daten(cfg, elektroanlage);

  const int speicher_stand = (float) data["SOC"];
  const int speicher_modus = (int) data["BatMode"];
  const int akku = round((float) data["P_Akku"]);

  const int load = round((float) data["P_Load"]) * -1;
  const int grid = round((float) data["P_Grid"]);
  const int solar = round((float) data["P_PV"]);
  const int max_i = (int) data["MAX_I"];
  const String max_i_phase = (String) data["MAX_I_PHASE"];
  if(!load && !grid && !solar && !max_i && !max_i_phase) {
    return Local::HTTPResponse(503, "text/plain", "<h1>503: Error</h1>");
  }

  const String speicher_ladung = content_converter.formatiere_zahl(false, Local::Config::speicher_groesse / 100 * speicher_stand);
  String speicher_status = "";
  if(speicher_modus != 1 && speicher_modus != 14) {
    speicher_status = "speicher_aus";
  }



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
    "setInterval(reload, " + (String) Local::Config::refresh_display_interval + " * 1000);\n"
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
    "<div style=\"display:none\"><span id=max_i_value>" + content_converter.formatiere_stromstaerke(max_i) + "</span><span id=max_i_phase>" + max_i_phase + "</span></div>"
    "<table cellspacing=0>"
    "<tr><td class=label>Solar</td><td class=zahl>" + content_converter.formatiere_zahl(false, solar) + "</td><td class=\"label einheit\">W</span></td></tr>"
    "<tr><td class=label>+Akku<br/></td><td class=zahl>" + content_converter.formatiere_zahl(true, akku) + "</td><td class=\"label einheit\">W</span></td></tr>"
    "<tr><td class=label>+Netz</td><td class=zahl>" + content_converter.formatiere_zahl(true, grid) + "</td><td class=\"label einheit\">W</span></td></tr>"
    "<tr class=last><td class=label>=Last</td><td class=zahl>" +  content_converter.formatiere_zahl(false, load) + "</td><td class=\"label einheit\">W</span></td></tr>"
    "<tr class=markerline><td colspan=3><span id=max_i></span><span id=marker>starte...</span><span id=fehler></span></td></tr>"
    "<tr class=\"speicher " + speicher_status + "\"><td class=label>Speicher</td><td class=zahl>" + speicher_ladung + "</td><td class=\"label einheit\">Wh</span></td></tr>"
    "</table>"
    "</body></html>";
  return Local::HTTPResponse(200, "text/html", html);
}
