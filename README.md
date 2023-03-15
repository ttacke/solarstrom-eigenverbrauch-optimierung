# Solarstrom-Eigenverbrauch-Optimierung

Steuerung, um den Solarstrom-Eigenverbrauch zu optimieren und die Solar-Batterie dabei
weitgehend schonend zu behandeln.

Konkret:
- Pufferbatterie beim Laden anderer Verbraucher am besten nur, wenn überhaupt, zwischen 40% und 60% zu "bewegen"
- Pufferbatterie-Zielladestand am Ende des Tages: 80%
- Bei drohender Einspeisung von frühzeitiges Überladen der Warmwasser- und Heizungsanlage

## Steuerungs-UI

![Screenshot der UI](ui-screenshot.png)

- Obere Zeile (v.l.n.r):
  - Überschuss-Leistung (Solarerzeugung abzüglich Hausverbrauch)
    - Klick auf die Zahl erzwingt eine sofortige Aktualisierung der Daten
  - Pufferakku-Ladestand
- Zweite Zeile (v.l.n.r; schwarz=aktiv, grau=inaktiv):
  - Status Solarerzeugung incl relative Verteilung der Leistung der beiden Dachhälften
  - Auto-Wallbox incl. Debug-Daten
    - Debug-Daten bedeuten:
      - Einschaltschwelle erreicht=1/0
      - Pufferakku-Zielladestand wird erreicht=1/0
      - Mindestschaltdauer erreicht=1/0
      - Aktuell verfügbare Leistung relativ zur benötigten Ladeleistung (1.0=100%) 
  - Roller-Lader incl. Debug-Daten
    - Debug-Daten: siehe Auto
  - Warmwasser-Überladen incl. Debug-Daten
    - Debug-Daten bedeuten:
      - Unerfüllter Ladewunsch existiert=1/0
      - Pufferakku wird überlaufen innerhalb von 3h=1/0
      - Mindestschaltdauer erreicht=1/0
      - Aktuell verfügbare Leistung relativ zur benötigten Ladeleistung (1.0=100%) 
  - Heizungs-Überladen incl. Debug-Daten
    - Debug-Daten: siehe Wasser
- Dritte Zeile (v.l.n.r):
  - Vorhersage des Pufferakku-Ladestandes der nächsten 12 Stunden
    - schwarze Balken bedeuten, das potentiell zu viel Energie vorhanden sein wird (=Netzeinspeisung)
  - Vorhersage der Sonnenenergie der nächsten 5 Tage
    - schwarze Balken bedeuten, das potentiell zu viel Energie vorhanden sein wird (=Netzeinspeisung)
- Unterer Bereich
  - Schalter der Ladesteuerung für Auto-Wallbox und Roller-Lader (schwarz = aktiv, weiß = inaktiv)
    - 5x tippen auf "Roller" bzw "Auto" schaltet die Ladeleistung um
      - es existiert kein Rückkanal für diese Info, so dass Änderungen an den Ladeleistungen hier manuell eingestellt werden müssen
  - &#9889; (Blitz) = Egal was ist kommt, es wird geladen
  - &#9728; (Sonne) = Verwende nur den Überschuss, der Zielladestand des Pufferakkus (80%) soll erreicht werden

## Systembestandteile:
- ein ESP8266-E12 als Zentrale
  - Dem wurde eine SD-Karte als Datenspeicher hinzugefügt 
    - ![Schaltplan zum Anschluss der SD-Karte an den ESP8266-12E](sd-card-anschlussplan.png)
    - Quelle: <https://www.mischianti.org/2019/12/15/how-to-use-sd-card-with-esp8266-and-arduino/#esp8266>
- 3x "3V Relais Power Switch Board"
  - Werden verwendet, um die Potentialfreihen eingänge der Wärmepumpen und der Wallbox zu schalten
  - Da die ESPs nur sehr wenig Leistung abgeben können, wird jedes Relais von einem weiteren ESP8266-E12 betrieben. Die Steuerung erfolgt via Netzwerk.
- 1x "Shelly Plug S"
  - Wird genutzt, um den Roller-Lader zu schalten
  - wird per WLAN gesteuert
- Ein (ausgedienter) KindlePaperwhite der ersten Generation für als Anzeige
  - Der genutzte Beta-Browser hat einige Eigenheiten, wewegen die UI auf anderen Geräte schräg aussieht
  - Auf dem Kindle wurde der Bildschirm dauerhaft eingeschaltet via "~ds" im Suchfeld eingeben
    - Quelle: <https://ebooks.stackexchange.com/questions/152/what-commands-can-be-given-in-the-kindles>
- Ein kostenloser Dev-Zugang zum Dienst AccuWeather.com, um Vorhersagen zu Solreinstrahlung zu erhalten
  - 50 Requests pro Tag sind erlaubt, das System benutzt 24 (Stündliche Vorhersage) + 4 (Tages-Vorhersage)
  - Die LocationID muss dazu ermitteln werden via <https://developer.accuweather.com/accuweather-locations-api/apis/get/locations/v1/search>
- Wechselrichter "Fronius Symo Gen 24 8.0 Plus" an 8,25 kWp monokristallinen Solarpanelen
- 7,65 kWh LiFePo4 Solarbatterie "BYD Battery-Box HVS 7,7" 
- Leistungsmessung via "Fronius Smart Meter"
- E-Auto an Wallbox "Süwag eBox AC11-SÜW"
  - via potentialfeiem Eingang sperrbar 
  - aka "Süwag eBox plus"
  - fast Baugleich zu "Vestel EVC04-AC11"
- Warmwasser-Wärmepumpe "Viessmann Vitocal 060-A, Typ TOS-ze Umluft 254L"
  - via potentialfeiem Eingang kann es "überladen" werden (Zieltemperatur wird temporär auf Maximum gesetzt)
  - PV-Anschluss Doku <https://www.viessmann-community.com/t5/Waermepumpe-Hybridsysteme/Funktion-PV-Anlage-mit-Vitocal-262-A-und-Vitocal-060-A/m-p/303739/emcs_t/S2h8ZW1haWx8dG9waWNfc3Vic2NyaXB0aW9ufExENDlMU0w2VVlVREtCfDMwMzczOXxTVUJTQ1JJUFRJT05TfGhL#M64397>

Kommt noch:
- Warmwasser-Wärmepumpe "Viessmann Vitocal 060-A, Typ TOE-ze Umluft 178L" als Heizung
  - Steuerung baugleich wie Warmwasser
- Heizungs-Mischer - noch unbekannt

## HINWEIS
Wechselrichter und Batterie sollten nie im Netzwerk mit anderen Endgeräten sein. Die Zugänge
sind fest codiert und "zertifizierten Handwerkern" bekannt. Also jedem. Jemand/Etwas auf einem
anderen Endgerät im gleichen Netzwerk könnte also problemlos Schaden anrichten. Und: SSL kennen
die Geräte nicht. Ebenso wenig kann den Zugangsbeschränkungen getraut werden - die interne API des
Wechselrichters komplett ohne Login benutzbar.
