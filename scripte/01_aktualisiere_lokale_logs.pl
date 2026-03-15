#!/usr/bin/perl
# Aufruf: cd /projekt/scripte && perl /mnt/scripte/ergaenze_fehlende_monate.pl [IP]
#
# Prueft alle Monate von der aeltesten lokalen Logdatei bis heute:
#   - Fehlende Monate werden heruntergeladen.
#   - Monate, deren Datei vor Monatsende heruntergeladen wurde (unvollstaendige Daten),
#     werden neu heruntergeladen (mit Backup der alten Datei).
use strict;
use warnings;

my $SERVER_IP;
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
    $SERVER_IP = '192.168.0.15';
    if($ARGV[0]) {
        $SERVER_IP = $ARGV[0];
    }
    print "Server-IP: $SERVER_IP\n";
}

use Time::Local;
use POSIX qw(strftime);

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
    opendir(my $dh, $SD_DIR) or die "Kann $SD_DIR nicht oeffnen: $!\n";
    my @files = sort grep { /^anlage_log-\d{4}-\d{2}\.csv$/ } readdir($dh);
    closedir($dh);
    return () unless @files;
    $files[0] =~ /(\d{4})-(\d{2})/;
    return ($1 + 0, $2 + 0);
}

my ($start_year, $start_month) = _finde_aeltesten_monat();
if(!$start_year) {
    die "Keine lokalen anlage_log-Dateien in $SD_DIR gefunden.\n";
}
printf("Aelteste Logdatei: %04d-%02d\n", $start_year, $start_month);

my @heute        = localtime();
my $current_year  = $heute[5] + 1900;
my $current_month = $heute[4] + 1;
printf("Aktueller Monat:   %04d-%02d\n\n", $current_year, $current_month);

my @fehlend  = ();  # [$year, $month]
my @unvollst = ();  # [$year, $month, $mtime_str, $monatsende_str]

my ($y, $m) = ($start_year, $start_month);
while($y < $current_year || ($y == $current_year && $m <= $current_month)) {
    my $filename = sprintf("anlage_log-%04d-%02d.csv", $y, $m);
    my $path     = "$SD_DIR/$filename";

    if(!-f $path || (stat($path))[7] == 0) {
        push(@fehlend, [$y, $m]);
    } elsif($y < $current_year || $m < $current_month) {
        # Abgeschlossener Monat: Datei muss nach dem letzten Tag des Monats heruntergeladen worden sein
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
    my ($fy, $fm) = @$entry;
    _download(sprintf("anlage_log-%04d-%02d.csv", $fy, $fm));
}

for my $entry (@unvollst) {
    my ($uy, $um) = @$entry;
    my $filename = sprintf("anlage_log-%04d-%02d.csv", $uy, $um);
    _backup_file($filename);
    _download($filename);
}

my $aktuell = sprintf("anlage_log-%04d-%02d.csv", $current_year, $current_month);
printf("Aktueller Monat wird immer aktualisiert:\n  %s...", $aktuell);
_backup_file($aktuell);
_download($aktuell);
