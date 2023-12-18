#!/usr/bin/perl
use strict;
use warnings;

my $SERVER_IP;
my $MONATE_IN_VERGANGENHEIT;
BEGIN {
    if(system("wget --version 1>/dev/null 2>&1") != 0) {
        print "Bitte das Tool 'wget' installieren\n";
        exit(1);
    }
    if(!eval {
        require Date::Calc;
        return 1;
    }) {
        print "Bitte die Bibliothek 'Date::Calc' installieren\n";
        exit(1);
    }
    $SERVER_IP = '192.168.0.30';
    if($ARGV[1]) {
        $SERVER_IP = $ARGV[1];
    }
    print "Benutzte IP des ESP8266-12E-Controllers(zentrale): $SERVER_IP\n";
    $MONATE_IN_VERGANGENHEIT = 0;
    if($ARGV[0]) {
        $MONATE_IN_VERGANGENHEIT = $ARGV[0] * 1;
    }
    print "Anzahl der Monate, die die Log aus der Vergangenheit geholt wird: $MONATE_IN_VERGANGENHEIT\n";
}
my @files = qw/
    system_status.csv
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
    auto_leistung.log
    verbrauch_leistung.log
    erzeugung_leistung.log
    frueh_laden_auto.status
    frueh_laden_roller.status
    ladeverhalten_wintermodus.status
/;
sub _backup_file {
    my ($filename) = @_;

    my $source = "../sd-karteninhalt/$filename";
    if(-f($source) && [stat($source)]->[7] > 0) {
        my $time = time();
        `cp $source ../sd-karteninhalt/$filename.bak.$time`;
    }
}
@files = ();
for(my $i = 0; $i <= $MONATE_IN_VERGANGENHEIT; $i++) {
    my @d = localtime();
    my $year = $d[5] + 1900;
    my $month = $d[4] + 1;
    ($year, $month, undef) = Date::Calc::Add_Delta_YM($year, $month, 1, 0, $i * -1);
    my $anlage_log = sprintf("anlage_log-%04d-%02d.csv", $year, $month);
    _backup_file($anlage_log);
    my $verbraucher_log = sprintf("verbraucher_automatisierung-%04d-%02d.log", $year, $month);
    _backup_file($verbraucher_log);
    push(@files, $anlage_log);
    push(@files, $verbraucher_log);
}
foreach my $filename (@files) {
    print "$filename...";
    my $target = "../sd-karteninhalt/$filename";
    if(system("wget --tries=1 --read-timeout=30 'http://$SERVER_IP/download_file?name=$filename' -O $target") == 0) {
        print "ok\n";
    } else {
        print "FEHLER\n";
    }
}

print "daten.json...";
my $target = "../sd-karteninhalt/daten.json";
if(system("wget --tries=1 --read-timeout=30 'http://$SERVER_IP/daten.json' -O $target") == 0) {
    print "ok\n";
} else {
    print "FEHLER\n";
}
