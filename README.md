# Solarstrom-Eigenverbrauch-Optimierung

Steuerung, um aus der solaren Stromerzeugung so viel Nutzen zu ziehen, wie irgendwie geht,
und die Solar-Batterie dabei weitgehend schonend behandeln.

Konkret:
- Pufferbatterie beim Laden anderer Verbraucher am besten zwischen 40% und 60% "bewegen"
- Pufferbatterie-Ladestände versuchen zwischen 20% und 80% zu halten
- Pufferbatterie-Zielladestand am Ende des Tages: 90%
  - Bei potentiellem Überschreiten des Pufferbatterie-Zielladestandes:
    - Laden von Auto und Roller
    - einmaliges, morgendliches (4 Uhr UTC) Leeren der Puffer-Batterie auf 25% in Auto und Roller, da beides potentiell tagsüber nicht da ist
- Bei potentieller Einspeisung:
  - frühzeitiges Überladen der Warmwasser- und Heizungsanlage
- Bei akuter Einspeisung:
  - aktivieren der Warmwasser-Begleitheizung
  - freischalten des Heizungs-Heizstabes
 
Zusätzliche Features:
- Optimierung des Energieverbrauches für die Heizung:
  - Wegen Fehlen einer Abtaueinrichtung an der Heizungs-Wärmepumpe gibt es einen externen Zuluft-Erwärmer. Der wird nur dann gesteuert, wenn
    die Abluft akutes einfrieren anzeigt.
  - Der Elektro-heizstab der Heizung wird nur dann freigeschaltet, wenn der Temperaturunterschied von Vor- und Rücklauf sehr gerung ist.
    Die Wärmepumpe startet ihn sonst schon bei <45° Kesseltemperatur, was unnötig ist.
- Winter-Ladeverhalten für Auto und Roller
  - wenn aktiviert, lädt das Auto immer Nachts zwischen 19 und 4 Uhr UTC
- Winter-Standort des Rollers
  - wenn aktiviert, wird die Keller-Steckdose genutzt, an der beide Akkus parallel geladen werden. Sonst die Außendose.
- Last wird abgeworfen bzw gar nicht erst zusgeschaltet um...
  - nur maximal 6,5kW aus dem Netz zu ziehen (vom Netzbetreiber vorgegeben)
  - den Wechselrichter nicht über 80% seiner Leistung zu betreiben
  - bei Ersatzstrom und keinem Überschuss nicht die Batterie leer zu saugen
- Bei Ersatzstrom ist nur Überschussladen möglich
  - dann wird der Akku-Zielladestand auf 120% gesetzt, so dass nur bei akutem Überschuss geladen wird
- Wenn die Steuerung deaktiviert wird, in dem einfach dessen Strom abgeschalten wird, verhält sich das System folgendermaßen:
  - die Wallbox ist immer an
  - es findet kein Solar-Überschussladen statt
  - der Roller-Lader ist dann aber dauerhaft aus und muss über eine normale Steckdose benutzt werden
  - die Warmwasser-Begleitheizung wird nur von 18-23 Uhr gestartet
  - der Heizungs-Heizstab wird von der Heizungs-Wärmepumpe gesteuert
  - der Heizungs-Zuluft-Erwärmer wird von seinem eingebauten Temperaturfühler gesteuert

## Steuerungs-UI (optional)

![Screenshot der UI](ui-screenshot.png)

- Obere Zeile:
  - Links: Überschuss-Leistung (Solarerzeugung abzüglich Hausverbrauch)
  - Rechts: Pufferakku-Ladestand
- Zweite Zeile (v.l.n.r; schwarz=aktiv, grau=inaktiv):
  - Status Solarerzeugung incl relative Verteilung der Leistung der beiden Dachhälften
  - Ladestatus von Auto-Wallbox, Roller-Lader, Warmwasser- und Heizungs-Überladen
    - Anzeige "schutz"/mit Unrandung = Lastschutz, dieser Ladewunsch wird blockiert
- Dritte Zeile:
  - Links: Vorhersage des Pufferakku-Ladestandes der nächsten 12 Stunden
  - Rechts: Vorhersage der Sonnenenergie der nächsten 5 Tage
  - Die Balken sind via Fibonacci-Folge gestaffelt. Helle Balken, ganz unten, stellen bis 20% Akkustand dar, die etwas dunkleren bis 100%. Schwarze Balken bedeuten, das potentiell zu viel Energie vorhanden sein wird (= passt nicht mehr in Pufferakku = Netzeinspeisung)
- Unterer Bereich: Schalter der Ladesteuerung
  - Schalter "Auto" und "Roller": tippen auf Button de/aktiviert ihn
    - aktiv = Wallbox wird aktiviert, außer Lastschutz sagt etwas anderes. Wechselt nach 12h wieder zu inaktiv.
    - inaktiv + Sonne und Mond sind zu sehen (1. Oktober - 14. März) = in Nicht-Hochlastzeiten wird geladen + Überschussladen 
    - inaktiv + nur Sonne ist zu sehen (15. April - 30. September) = Überschussladen
  - Nur bei "Roller"
    - "Keller": beide Akkus ja an einem Ladegerät an der Kellerdose
    - "Außen": beide Akkus im Roller mit nur einem Ladegerät (wird automatisch aktiviert, wenn die Ladebuchse außen an ist)
- ganz unten: Temperatur im Keller (nur für Betrieb der Heizanlage zur Überwachung)

## Systembestandteile:
- ein ESP8266-E12 als Zentrale
  - Dem wurde eine SD-Karte als Datenspeicher hinzugefügt 
    - ![Schaltplan zum Anschluss der SD-Karte an den ESP8266-12E](sd-card-anschlussplan.png)
    - Quelle: <https://www.mischianti.org/2019/12/15/how-to-use-sd-card-with-esp8266-and-arduino/#esp8266>
- 3x "3V Relais Power Switch Board"
  - um die potentialfreihen Eingänge der Wärmepumpen und der Wallbox zu schalten
  - Da die ESPs nur sehr wenig Leistung abgeben können, wird jedes Relais von einem weiteren ESP8266-E12 betrieben. Die Steuerung erfolgt via Netzwerk.
- 1x "Shelly 1 Mini"
  - um den Heizungs-Elektro-Heizstab zu sperren bzw freizugeben
- 4x "Shelly Plug S", 1x "Shelly-Plug"
  - um den Roller-Lader (1x innen und 1x aussen), die Warmwasser-Begleitheizung (1x), den Heizungs-Zuluft-Erwärmer (1x) zu schalten
  - um die Aktivität der Warmwasser-Wärmepumpe (1x) zu messen
  - wird per WLAN gesteuert
- Ein (ausgedienter) KindlePaperwhite der ersten Generation als Anzeige
  - Der genutzte Beta-Browser hat einige Eigenheiten, wewegen die UI auf anderen Geräte schräg aussieht
  - Auf dem Kindle wurde der Bildschirm dauerhaft eingeschaltet via "~ds" im Suchfeld eingeben
    - Quelle: <https://ebooks.stackexchange.com/questions/152/what-commands-can-be-given-in-the-kindles>
- Ein kostenloser Zugang zum Dienst api.open-meteo.com, um Vorhersagen zu Solareinstrahlung zu erhalten
  - 10k Requests pro Tag sind erlaubt - hier werden nur 24, eine pro Stunde, verwendet
  - Lon und Lat muss mit 2 Kommastellen ermittelt werden
  - Neigung (in Grad) und Ausrichtung (S = 0°, O = -90°, W = 90°, N = -/+180°) der Dachflächen, 2x für die 2 Flächen, muss ermittelt werden
- Wechselrichter "Fronius Symo Gen 24 8.0 Plus" an 8,25 kWp monokristallinen Solarpanelen
- 7,65 kWh LiFePo4 Solarbatterie "BYD Battery-Box HVS 7,7" 
- Leistungsmessung via "Fronius Smart Meter"
- E-Auto an Wallbox "Süwag eBox AC11-SÜW"
  - via potentialfeiem Eingang sperrbar 
  - aka "Süwag eBox plus"
  - fast Baugleich zu "Vestel EVC04-AC11"
- Warmwasser-Wärmepumpe "Viessmann Vitocal 060-A, Typ TOS-ze Umluft 254L"
  - via potentialfeiem Eingang kann es "überladen" werden (Zieltemperatur wird temporär auf Maximum gesetzt)
  - [PV-Anschluss Doku](https://www.viessmann-community.com/t5/Waermepumpe-Hybridsysteme/Funktion-PV-Anlage-mit-Vitocal-262-A-und-Vitocal-060-A/m-p/303739/emcs_t/S2h8ZW1haWx8dG9waWNfc3Vic2NyaXB0aW9ufExENDlMU0w2VVlVREtCfDMwMzczOXxTVUJTQ1JJUFRJT05TfGhL#M64397)
- Heizungs-Wärmepumpe "Viessmann Vitocal 060-A, Typ TOE-ze Umluft 178L"
  - via potentialfeiem Eingang kann es "überladen" werden (Zieltemperatur wird temporär auf Maximum gesetzt)
  - [PV-Anschluss Doku](https://www.viessmann-community.com/t5/Waermepumpe-Hybridsysteme/Funktion-PV-Anlage-mit-Vitocal-262-A-und-Vitocal-060-A/m-p/303739/emcs_t/S2h8ZW1haWx8dG9waWNfc3Vic2NyaXB0aW9ufExENDlMU0w2VVlVREtCfDMwMzczOXxTVUJTQ1JJUFRJT05TfGhL#M64397)
  - Der integrierte Elektro-Heizstab ist nachträglich mit einem Unterbrecher versehen worden

## Einrichtung
- Die Zentrale mit der SD-Karte versehen
- Die Netz-Relay-ESPs mit den Relays verbinden und an die passenden, potentialfreien Eingänge hängen
- Die Shelly-Plug im Netzwerk einrichten
- [ArduinoIDE](https://www.arduino.cc/en/software) installieren
  - Dort den [Boardmanager](http://arduino.esp8266.com/stable/package_esp8266com_index.json) einfügen (siehe [Anleitung](https://circuitjournal.com/esp8266-with-arduino-ide))
  - in der Bibliotheksverwaltung die Module "SD", "Regexp" und "Time" installieren
- Die IPs der ESPs im Netzwerk einrichten, dass diese immer auf die gleiche Art erreichbar sind
- unter /netz-relay und /zentrale jeweils die _TEMPLATE.config.h in config.h kopieren und dort sinnvolle Werte eintragen
  - nach 14 Tagen Laufzeit kann man mit diesen Scripten ermitteln lassen, wie der "grundverbrauch_in_w_pro_h_sommer/winter". Werte für "solarstrahlungs_vorhersage_umrechnungsfaktor_sommer/winter" wird nach ca. einem Monat irgend etwas stabiles ergeben. Erst nach einem Jahr können sichere Werte ermittelt werden.
    ```
    cd /script/
    perl download_der_daten_der_zentrale.pl [IP-DER-ZENTRALE]
    perl ermittle_statistik.pl [IP-DER-ZENTRALE]
    ```
- In der IDE das Script netz-relay/netz-relay.ino öffnen und auf die Netz-Relay-ESPs spielen
- In der IDE das Script zentrale/zentrale.ino öffnen und auf das zentrale Steuer-ESP spielen
- Option1: Auf z.B. dem KindlePaperwhite die UI der Zentrale aufrufen http://[IP-DER-ZENTRALE]/
  - Das forcierte Laden von Rollerund Autokann über den dortigen Button gesteuert werden
- Option2: mit z.B. einem ShellyButton die URL der Zentrale aufrufen, um das Laden zu forcieren (erzwungenes Volladen, egal was die Daten sagen)
  - forciertes Laden des Autos: http://[IP-DER-ZENTRALE]/change?key=auto&value=force
    - wieder zurücksetzen: http://[IP-DER-ZENTRALE]/change?key=auto&value=solar
  - forciertes Laden des Rollers: http://[IP-DER-ZENTRALE]/change?key=roller&value=force
    - wieder zurücksetzen: http://[IP-DER-ZENTRALE]/change?key=roller&value=solar

## HINWEIS
Wechselrichter und Batterie sollten nie im Netzwerk mit anderen Endgeräten sein. Die Zugänge
sind fest codiert und "zertifizierten Handwerkern" bekannt. Also jedem. Jemand/Etwas auf einem
anderen Endgerät im gleichen Netzwerk könnte also problemlos Schaden anrichten. Und: SSL kennen
die Geräte nicht.

