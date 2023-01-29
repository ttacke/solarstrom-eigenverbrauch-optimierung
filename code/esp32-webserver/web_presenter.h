#pragma once
#include "config.h"
#include "wlan.h"
#include "webserver.h"
#include "web_client.h"
#include "persistenz.h"
#include "formatierer.h"
#include "elektro_anlage.h"
#include "smartmeter_leser.h"
#include "wechselrichter_leser.h"
#include "wettervorhersage_leser.h"

namespace Local {
	class WebPresenter {
	protected:
		Local::Config cfg;

		Local::ElektroAnlage elektroanlage;
		Local::Persistenz persistenz;
		Local::WebClient web_client;

		void s(String content) {// TODO besser von SD laden!
			webserver.server.sendContent(content);
		}

	public:
		Local::Webserver webserver;

		WebPresenter(
			Local::Config& cfg, Local::Wlan& wlan
		): cfg(cfg), web_client(wlan.client), webserver(cfg.webserver_port) {
		}

		void zeige_hauptseite() {
			Local::WechselrichterLeser wechselrichter_leser(cfg, web_client);
			wechselrichter_leser.daten_holen_und_einsetzen(elektroanlage);

			Local::SmartmeterLeser smartmeter_leser(cfg, web_client);
			smartmeter_leser.daten_holen_und_einsetzen(elektroanlage);

			Local::WettervorhersageLeser wetter_leser(cfg, web_client);
			wetter_leser.daten_holen_und_einsetzen(elektroanlage);

			persistenz.append2file((char*) "anlage.log", elektroanlage.gib_log_zeile());

			webserver.server.setContentLength(CONTENT_LENGTH_UNKNOWN);
			webserver.server.send(200, "text/html", "");
			s("<html><head><title>Sonnenrech 19</title>"
				"<script type=\"text/javascript\">"
				"var anzahl_fehler = 0;\n"
				"function setze_lokale_daten() {\n"
				"  var max_i = document.getElementById('max_i_value').innerHTML * 1;\n"
				"  if(!document.max_i || document.max_i < max_i) {\n"
				"   document.max_i = max_i * 1;\n"
				"   document.max_i_phase = document.getElementById('max_i_phase').innerHTML;\n"
				"   var now = new Date();\n"
				"   document.max_i_uhrzeit = now.getHours() + ':' + (now.getMinutes() < 10 ? '0' : '') + now.getMinutes();\n"
				"  }\n"
				"  document.getElementById('marker').innerHTML = document.max_i_uhrzeit;\n"
				"  document.getElementById('max_i').innerHTML = 'max I=' + document.max_i + 'A&nbsp(' + document.max_i_phase + ')&nbsp;';\n"
				"}\n"
			);
			s("function reload() {\n"
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
			);
			s("setInterval(reload, " + (String) cfg.refresh_display_interval + " * 1000);\n"
				"</script>"
				"<style>"
				"body{padding:0.5rem;padding-top:0.5rem;}"//transform:rotate(180deg); geht im Kindle nicht
				"*{font-family:sans-serif;font-weight:bold;color:#000;}"
				"td{font-size:5rem;line-height:1.1;}"
				"table{width:100%;}"
				"tr.last td {border-top:5px solid #000;}"
				".zahl{text-align:right;padding-left:0.3rem;}"
				".label{font-size:1.5rem;text-align:right;}"
				".einheit{padding-left:0.3rem;width:0%;}"
				".markerline div{text-align:center;font-size:2.5rem;}"
				".markerline div span{color:#999;}"
				".markerline div #max_i{}"
				".markerline div #marker{}"
				".speicher_aus td{color:#999;}"
				".hat_fehler *{color:#999;}"
				".hat_fehler .markerline div span{color:#000;text-decoration:underline;}"
				"</style>"
			);
			s("</head><body onclick=\"reload();\" onload=\"setze_lokale_daten();\">"
				"<div style=\"display:none\"><span id=max_i_value>" + Local::Formatierer::format((char*) "%.1f", (float) elektroanlage.max_i_in_ma() / 1000) + "</span><span id=max_i_phase>" + elektroanlage.max_i_phase() + "</span></div>"
				"<table cellspacing=0 border=1>"
				"<tr>"
					"<td class=zahl style=\"width:auto\">" + Local::Formatierer::wert_als_k(false, elektroanlage.gib_ueberschuss_in_wh()) + "</td>"
					"<td class=\"label einheit\" style=\"width:0.5rem\">W</span></td>"
					"<td class=zahl style=\"width:1.8em;font-size:3.5rem;\">" + Local::Formatierer::format((char*) "%.1f", (float) elektroanlage.solarakku_ladestand_in_promille / 10) + "</td>"
					"<td class=\"label einheit\" style=\"width:0.5rem\">%</span></td>"
				"</tr>"
		//		"<tr><td class=label>+Akku<br/></td><td class=zahl>" + Local::Formatierer::wert_als_k(true, elektroanlage.solarakku_zuschuss_in_wh) + "</td><td class=\"label einheit\">W</span></td></tr>"
		//		"<tr><td class=label>+Netz</td><td class=zahl>" + Local::Formatierer::wert_als_k(true, elektroanlage.netzbezug_in_wh) + "</td><td class=\"label einheit\">W</span></td></tr>"
		//		"<tr class=last><td class=label>=Last</td><td class=zahl>" + Local::Formatierer::wert_als_k(false, elektroanlage.stromverbrauch_in_wh * -1) + "</td><td class=\"label einheit\">W</span></td></tr>"
		//		"<tr class=markerline><td colspan=3><span id=max_i></span><span id=marker>starte...</span><span id=fehler></span></td></tr>"
		//		"<tr class=\"speicher " + (elektroanlage.solarakku_ist_an ? "" : "speicher_aus") + "\"><td class=label>Speicher</td><td class=zahl>" + Local::Formatierer::wert_als_k(false, cfg.speicher_groesse / 100 * elektroanlage.solarakku_ladestand_in_promille) + "</td><td class=\"label einheit\">%</span></td></tr>"
				"</table>"
				"<div class=markerline><div><span id=max_i></span><span id=marker>starte...</span><span id=fehler></span></div></div>"
				"</body></html>"
			);
		}
	};
}
