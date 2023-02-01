# Solarstrom-Eigenverbrauch-Optimierung
Name: ???

Steuerung für die Haus-Elektrik, um Solarstrom-Eigenverbrauch zu optimieren und die Solar-Batterie dabei
weitgehend schonend zu behandeln.

Aktuell im System wird folgendes genutzt:
- Wechselrichter "Fronius Symo Gen 24 8.0 Plus" an 8,25 kWp monokristallinen Solarpanelen
- 7,7 kWp Solarbatterie "BYD Battery-Box HVS 7,7" mit LiFePo4
- Leistungsmessung via "Fronius Smart Meter"
- VW-ID3 an Wallbox "Süwag eBox AC11-SÜW" aka "Süwag eBox plus"
  - fast Baugleich zu "Vestel EVC04-AC11"
- Warmwasser-Wärmepumpe "Viessmann Vitocal 060-A, Typ TOE-ze Umluft 254L"

Kommt noch:
- Heizungs-Wärmepumpe via "Viessman Vitocal 060-A, Typ T0E-ze Umluft 180l"
- Heizungs-Mischer - noch unbekannt


## Dokus
- WWWP-PV-Anschluss https://www.viessmann-community.com/t5/Waermepumpe-Hybridsysteme/Funktion-PV-Anlage-mit-Vitocal-262-A-und-Vitocal-060-A/m-p/303739/emcs_t/S2h8ZW1haWx8dG9waWNfc3Vic2NyaXB0aW9ufExENDlMU0w2VVlVREtCfDMwMzczOXxTVUJTQ1JJUFRJT05TfGhL#M64397

## WICHTIG
Wechselrichter und Batterie sollten nie im Netz mit anderen Endgeräten sein. Die Zugänge
sind fest codierte und "zertifizierten Handwerkern" bekannt. Also jedem. Ein Virus auf einem
anderen Endgerät im gleichen Netz könnte also problemlos Schaden anrichten.

## TODO
- Wallboxnutzung optimieren.
  - Simpel: wenn SolarBatterie voll dann erst laden (verringert sicher überschuss, belastet Solarbatterie)
  - Besser: aktivierbare Steuerung des Ladevorganges: wenn Auto angeschlossen und nicht voll, dann...
    - SolarBatterie > 95% + Sonnenuntergang noch weiter weg + Überschuss = starte laden
    - Überschuss > Ladeleistung = starte laden
- WP einschalten, wenn Überschuss
  - Das gleiche, wie mit der Wallbox
  - hängt das Auto dran und ist nicht voll, dann die WP nachrangig nutzen (weil ineffizient in dem Modus)

  - Einstellung beim Bauen:
MMU -> 16kb + 48b IRAM

forecast.json:city.sunrise
forecast.json:city.sunset

ESPWebClient -> StreamHTTPClient Beispiel -> blockweise GET lesen

// Webserver fuer Anzeige auf einem KindleReader
// CSS/JS ist auf den dortigen "BetaBrowser" angepasst

// Hinweis: auf dem Kindle den Bildschirm dauerhaft einschalten: "~ds" im Suchfeld eingeben
// https://ebooks.stackexchange.com/questions/152/what-commands-can-be-given-in-the-kindles-search-box

// Es ist "Selbstheilend": Wenn der ESP nicht antwortet, wird die anzeige ausgegraut und beim nächsten Abruf
// einfach erneut versucht

## SD Verdrahtung

![Schaltplan zum Anschluss der SD-Karte an den ESP8266-12E](sd-card-anschlussplan.png)
Quelle: <https://www.mischianti.org/2019/12/15/how-to-use-sd-card-with-esp8266-and-arduino/#esp8266>

