# Solarstrom-Eigenverbrauch-Optimierung
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
- warum ist max-I immer bei 9,9? Macht der einen Stringvergleich statt zahlen?
- die Verrechnung mit 3 abrufen entfernen
- Nachts deaktivieren und auch keine Abrufe machen, außer Auto lädt
  - Dazu muss aber der Sonnenauf/Untergang ermittelt werden? Im Netz ggf?
  - Oder intern speichern, wann Gestern Sonnenauf/Untergang war. Das ändert sich ja nur langsam und der Solarertrag=0 ist ja genau der Indikator, den es braucht
- Vorhersage des Solarertrages. Wo gibts Sonnenscheindaten? Für die nächsten Stunden reicht ja
  - Ist das nötig? Das Ziel ist ja Eigentnutzung. Da braucht es keine Vorhersage. Aber der Jahresverlauf wirds zeigen
- Wallboxnutzung optimieren.
  - Simpel: wenn SolarBatterie voll dann erst laden (verringert sicher überschuss, belastet Solarbatterie)
  - Besser: aktivierbare Steuerung des Ladevorganges: wenn Auto angeschlossen und nicht voll, dann...
    - SolarBatterie > 95% + Sonnenuntergang noch weiter weg + Überschuss = starte laden
    - Überschuss > Ladeleistung = starte laden
- WP einschalten, wenn Überschuss
  - Das gleiche, wie mit der Wallbox
  - hängt das Auto dran und ist nicht voll, dann die WP nachrangig nutzen (weil ineffizient in dem Modus)
- Aufzeichnung des Lastprofiles (bzw aller Daten, die da sind; immer Rohdaten nutzen)
  - 1x die Minute refresh, und die Zahlen in Datei schreiben
  - Eine grobe UI dazu kann auch eingebaut werden. Aber erst mal: haben!
