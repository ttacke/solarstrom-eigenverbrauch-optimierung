# Solarstrom-Eigenverbrauch-Optimierung

Steuerung für die Haus-Elektrik, um Solarstrom-Eigenverbrauch zu optimieren und die Solar-Batterie dabei
weitgehend schonend zu behandeln.

Aktuell im System wird folgendes genutzt:
- ein ESP8266-E12 als Zentrale
  - Dem wurde eine SD-Karte als Datenspeicher hinzugefügt 
    - ![Schaltplan zum Anschluss der SD-Karte an den ESP8266-12E](sd-card-anschlussplan.png)
    - Quelle: <https://www.mischianti.org/2019/12/15/how-to-use-sd-card-with-esp8266-and-arduino/#esp8266>
- Ein KindlePaperwhite der ersten Generation dient als Anzeige
  - Der genutzte Beta-Browser hat einige Eigenheiten, wewegen die UI auf anderen Geräte schräg aussieht
  - Auf dem Kindle wurde der Bildschirm dauerhaft eingeschaltet via "~ds" im Suchfeld eingeben
    - Quelle: <https://ebooks.stackexchange.com/questions/152/what-commands-can-be-given-in-the-kindles>
- Ein kostenloser Dev-Zugang zum Dienst AccuWeather.com, um Vorhersagen zu Solreinstrahlung zu erhalten
  - 50 Requests pro Tag sind erlaubt, das System benutzt 24 (Stündliche Vorhersage) + 4 (Tages-Vorhersage)
  - Die LocationID muss dazu ermitteln werden via <https://developer.accuweather.com/accuweather-locations-api/apis/get/locations/v1/search>
- Wechselrichter "Fronius Symo Gen 24 8.0 Plus" an 8,25 kWp monokristallinen Solarpanelen
- 7,7 kWp Solarbatterie "BYD Battery-Box HVS 7,7" mit LiFePo4
- Leistungsmessung via "Fronius Smart Meter"
- VW-ID3 an Wallbox "Süwag eBox AC11-SÜW" aka "Süwag eBox plus"
  - fast Baugleich zu "Vestel EVC04-AC11"
- Warmwasser-Wärmepumpe "Viessmann Vitocal 060-A, Typ TOE-ze Umluft 254L"
  - PV-Anschluss <https://www.viessmann-community.com/t5/Waermepumpe-Hybridsysteme/Funktion-PV-Anlage-mit-Vitocal-262-A-und-Vitocal-060-A/m-p/303739/emcs_t/S2h8ZW1haWx8dG9waWNfc3Vic2NyaXB0aW9ufExENDlMU0w2VVlVREtCfDMwMzczOXxTVUJTQ1JJUFRJT05TfGhL#M64397>

Kommt noch:
- Heizungs-Wärmepumpe via "Viessman Vitocal 060-A, Typ T0E-ze Umluft 180l"
- Heizungs-Mischer - noch unbekannt

## HINWEIS
Wechselrichter und Batterie sollten nie im Netz mit anderen Endgeräten sein. Die Zugänge
sind fest codiert und "zertifizierten Handwerkern" bekannt. Also jedem. Ein Virus auf einem
anderen Endgerät im gleichen Netz könnte also problemlos Schaden anrichten. Und: SSL kennen
die Geräte gar nicht.
