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
/) {
    print "$filename...";
    if(system("wget -q 'http://$ARGV[0]/download_file?name=$filename' -O ../zentrale/sd-karteninhalt/$filename") == 0) {
        print "ok\n";
    } else {
        print "FEHLER\n";
    }
}
print "daten.json...";
if(system("wget 'http://$ARGV[0]/daten.json?time=" . time() . "' -O ../zentrale/sd-karteninhalt/daten.json") == 0) {
    print "ok\n";
} else {
    print "FEHLER\n";
}
