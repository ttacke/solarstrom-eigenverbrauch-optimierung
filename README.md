# power-supply-control
Brings together a solar system with a batterie, a wallbox, two heatpumps and solar yield forecast

# WICHTIG
Wechselrichter und Batterie sollten nie im Netz mit anderen Endgeräten sein. Die Zugänge
sind fest codierte und "zertifizierten Handwerkern" bekannt. Also jedem. Ein Virus auf einem
anderen Endgerät im gleichen Netz könnte also problemlos Schaden anrichten.

# TODO
- warum ist max-I immer bei 9,9?
- die Verreichnung mit 3 abrufen entfernen
- Nachts deaktivieren und auch keine Abrufe machen
  - Dazu muss aber der Sonnenauf/Untergang ermittelt werden. Im Netz ggf?
- Vorhersage des Solarertrages. Wo gibts Sonnenscheindaten? Für die nächsten Stunden reicht ja
- Wallboxnutzung an Solarertrag anpassen.
  - Es soll nur verhindert werden, das Strom eingespeist wird
  - Solarertrags-Vorhersage <= PufferBatterieKapazität = Nix machen
  - Ansonsten muss der Ladezeitraum ja nur so gestreckt werden, dass nicht Anfangs Netzenergie genutzt wird und am Ende Solarenergie vergeudet wird
  - Default: 3,7kW von 10-16 Uhr
  - Eigentlich soll das Laden ja "nur" in den Sonnenschein geschoben werden und die Ladekurve nicht zu hoch
    - Wenn bis Sonnenuntergang NICHT voll, dann volle kanne weitermachen (weil effizienter Laden, gerade im Winter)
    - Wenn vor Sonnenuntergang voll und batterie potentiell auch voll wird, dann Laden verringern um Zeit zu verlängern
        - Nur, wenn Sonnentrom mehr als verringertes Laden und mit verringertem Laden auch Beides voll wird
           - D.h. nur wenn diese Bedingung greift, dann eingreifen 
- WP einschalten, wenn Überschuss
  - 500 Watt und sonst alles voll
  - ggf Batterie bei >90%?
