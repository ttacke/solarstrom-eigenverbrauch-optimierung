#!/usr/bin/perl
use strict;
use warnings;

if(!$ARGV[0]) {
    print "Bitte die IP des ESP8266-12E-Controllers als Parameter angeben\n";
    exit(1);
}
if(system("wget --version 1>/dev/null 2>&1") != 0) {
    print "Bitte das Tool 'wget' installieren\n";
    exit(1);
}

foreach my $filename (qw/
    system_status.csv
    anlage_log.csv
    wetter_stundenvorhersage.json
    wetter_stundenvorhersage.csv
    wetter_tagesvorhersage.json
    wetter_tagesvorhersage.csv
    heizung_relay.status
    wasser_relay.status
    roller_relay.zustand_seit
    roller.ladestatus
    roller_leistung.status
    roller_leistung.log
    auto_relay.zustand_seit
    auto.ladestatus
    auto_leistung.status
    auto_leistung.log
    ueberschuss_leistung.log
    verbraucher_automatisierung.log
/) {
    print "$filename...";
    if(system("wget -q 'http://$ARGV[0]/download_file?name=$filename' -O ../sd-karteninhalt/$filename") == 0) {
        print "ok\n";
    } else {
        print "FEHLER\n";
    }
}
