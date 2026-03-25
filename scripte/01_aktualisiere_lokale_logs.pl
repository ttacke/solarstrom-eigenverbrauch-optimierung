#!/usr/bin/perl
# Aktualisiert lokale Kopien der SD-Karten-Daten vom ESP8266.
#
# Monatslogs (anlage_log, verbraucher_automatisierung):
#   - Fehlende Monate werden heruntergeladen.
#   - Unvollstaendige abgeschlossene Monate werden neu heruntergeladen (mit Backup).
#   - Aktueller Monat wird immer neu heruntergeladen (mit Backup).
#
# Einzeldateien: werden immer ueberschrieben (kein Backup).
#
# Aufruf: cd /projekt/scripte && perl 01_aktualisiere_lokale_logs.pl [IP]

use strict;
use warnings;
use Time::Local;
use POSIX qw(strftime);

my $SERVER_IP;
BEGIN {
    if(system("wget --version 1>/dev/null 2>&1") != 0) {
        print "Bitte das Tool 'wget' installieren\n";
        exit(1);
    }
    if(!eval { require Date::Calc; 1 }) {
        print "Bitte die Bibliothek 'Date::Calc' installieren\n";
        exit(1);
    }
    $SERVER_IP = '192.168.0.15';
    if($ARGV[0]) {
        $SERVER_IP = $ARGV[0];
    }
    print "Server-IP: $SERVER_IP\n";
}

my $SD_DIR = '../sd-karteninhalt';

sub _backup_file {
    my ($filename) = @_;
    my $source = "$SD_DIR/$filename";
    if(-f $source && (stat($source))[7] > 0) {
        my $time = time();
        `cp $source $source.bak.$time`;
    }
}

sub _download {
    my ($filename) = @_;
    my $target = "$SD_DIR/$filename";
    print "$filename...";
    if(system("wget --tries=1 --read-timeout=30 'http://$SERVER_IP/download_file?name=$filename' -O $target") == 0) {
        print "ok\n";
        return 1;
    }
    print "FEHLER\n";
    return 0;
}

sub _finde_aeltesten_monat {
    my ($file_re) = @_;
    opendir(my $dh, $SD_DIR) or die "Kann $SD_DIR nicht oeffnen: $!\n";
    my @files = sort grep { /$file_re/ } readdir($dh);
    closedir($dh);
    return () unless @files;
    $files[0] =~ /(\d{4})-(\d{2})/;
    return ($1 + 0, $2 + 0);
}

sub _aktualisiere_monatslogs {
    my ($label, $name_tmpl, $file_re) = @_;

    print "\n=== $label ===\n";

    my @heute         = localtime();
    my $current_year  = $heute[5] + 1900;
    my $current_month = $heute[4] + 1;

    my ($start_year, $start_month) = _finde_aeltesten_monat($file_re);
    if(!$start_year) {
        print "Keine lokalen Dateien gefunden, lade nur aktuellen Monat.\n";
        ($start_year, $start_month) = ($current_year, $current_month);
    } else {
        printf("Aelteste Datei:  %04d-%02d\n", $start_year, $start_month);
        printf("Aktueller Monat: %04d-%02d\n\n", $current_year, $current_month);
    }

    my @fehlend  = ();
    my @unvollst = ();

    my ($y, $m) = ($start_year, $start_month);
    while($y < $current_year || ($y == $current_year && $m <= $current_month)) {
        my $filename = sprintf($name_tmpl, $y, $m);
        my $path     = "$SD_DIR/$filename";

        if(!-f $path || (stat($path))[7] == 0) {
            push(@fehlend, [$y, $m]);
        } elsif($y < $current_year || $m < $current_month) {
            my ($next_y, $next_m) = $m == 12 ? ($y + 1, 1) : ($y, $m + 1);
            my $cutoff_ts    = timelocal(0, 0, 0, 1, $next_m - 1, $next_y - 1900);
            my $actual_mtime = (stat($path))[9];
            if($actual_mtime < $cutoff_ts) {
                my $mtime_str = strftime('%Y-%m-%d', localtime($actual_mtime));
                my $days      = Date::Calc::Days_in_Month($y, $m);
                my $end_str   = sprintf('%04d-%02d-%02d', $y, $m, $days);
                push(@unvollst, [$y, $m, $mtime_str, $end_str]);
            }
        }

        ($y, $m) = (Date::Calc::Add_Delta_YM($y, $m, 1, 0, 1))[0, 1];
    }

    if(!@fehlend && !@unvollst) {
        print "Alle vergangenen Monate vorhanden und vollstaendig.\n";
    }
    if(@fehlend) {
        print "Fehlende Monate (" . scalar(@fehlend) . "):\n";
        printf("  %04d-%02d\n", $_->[0], $_->[1]) for @fehlend;
        print "\n";
    }
    if(@unvollst) {
        print "Unvollstaendige Monate (" . scalar(@unvollst) . ") - Dateidatum liegt vor Monatsende:\n";
        printf("  %04d-%02d  heruntergeladen: %s  Monatsende: %s\n",
            $_->[0], $_->[1], $_->[2], $_->[3]) for @unvollst;
        print "\n";
    }

    print "Starte Downloads...\n";
    for my $entry (@fehlend) {
        _download(sprintf($name_tmpl, $entry->[0], $entry->[1]));
    }
    for my $entry (@unvollst) {
        my $filename = sprintf($name_tmpl, $entry->[0], $entry->[1]);
        _backup_file($filename);
        _download($filename);
    }

    my $aktuell = sprintf($name_tmpl, $current_year, $current_month);
    printf("Aktueller Monat wird immer aktualisiert: %s\n", $aktuell);
    _backup_file($aktuell);
    _download($aktuell);
}

_aktualisiere_monatslogs(
    'anlage_log',
    'anlage_log-%04d-%02d.csv',
    qr/^anlage_log-\d{4}-\d{2}\.csv$/,
);

_aktualisiere_monatslogs(
    'verbraucher_automatisierung',
    'verbraucher_automatisierung-%04d-%02d.log',
    qr/^verbraucher_automatisierung-\d{4}-\d{2}\.log$/,
);

# Einzeldateien: immer ueberschreiben, kein Backup
print "\n=== Einzeldateien ===\n";
my @einzeldateien = qw(
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
);
_download($_) for @einzeldateien;

# daten.json hat eine andere URL als die uebrigen Dateien
print "daten.json...";
my $target = "$SD_DIR/daten.json";
if(system("wget --tries=1 --read-timeout=30 'http://$SERVER_IP/daten.json' -O $target") == 0) {
    print "ok\n";
} else {
    print "FEHLER\n";
}

# Backups komprimieren
print "\n=== Backups komprimieren ===\n";
my @backups = glob("$SD_DIR/*.bak.*");
if(@backups) {
    for my $bak (@backups) {
        next if $bak =~ /\.gz$/;
        print "$bak...";
        if(system("gzip", $bak) == 0) {
            print "ok\n";
        } else {
            print "FEHLER\n";
        }
    }
} else {
    print "Keine Backups gefunden.\n";
}
