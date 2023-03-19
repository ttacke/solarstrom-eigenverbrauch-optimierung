#!/usr/bin/perl
use strict;
use warnings;

if(system("wget --version 1>/dev/null 2>&1") != 0) {
    print "Bitte das Tool 'wget' installieren\n";
    exit(1);
}
if(!$ARGV[0]) {
    $ARGV[0] = '192.168.0.30';
    print "Benutzte IP des ESP8266-12E-Controllers(zentrale): $ARGV[0]\n";
}
sub _correct_download_errors {# TODO was ist eigentlich die Ursache des Fehlers?
    my ($filename) = @_;
    open(my $fh, '<', $filename) or die $!;
    local $/ = undef;
    my $content = <$fh>;
    close($fh);
    $content =~ s/[\n\r]+3f[\n\r]+//gsm;
    open($fh, '>', $filename) or die $!;
    print $fh $content;
    close($fh);
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
    verbrauch_leistung.log
    erzeugung_leistung.log
    verbraucher_automatisierung.log
/) {
    print "$filename...";
    my $target = "../sd-karteninhalt/$filename";
    if(system("wget -q 'http://$ARGV[0]/download_file?name=$filename' -O $target") == 0) {
        _correct_download_errors($target);
        print "ok\n";
    } else {
        print "FEHLER\n";
    }
}

print "daten.json...";
my $target = "../sd-karteninhalt/daten.json";
if(system("wget -q 'http://$ARGV[0]/daten.json?time=" . time() . "' -O $target") == 0) {
    _correct_download_errors($target);
    print "ok\n";
} else {
    print "FEHLER\n";
}
