#!/usr/bin/perl
use strict;
use warnings;

my $daten = [];
open(my $fh, '<', './anlagen.log') or die "Bitte erst 'lade_aktuelle_logdatei.pl [ESP-IP]' ausfuehren\n";
$/ = "\n";
while(my $line = <$fh>) {
    chomp($line);

    my @e = $line =~ m/^(\d+),e1,([\-\d]+),([\-\d]+),(\d+),(\d+),(\d+),([\-\d]+),([\-\d]+),([\-\d]+),(\d+),w1,(\d+),(\d+)$/;
    next if(!scalar(@e));

    push(@$daten, {
        zeitpunkt => $e[0],
        netzbezug_in_wh => $e[1],
        solarakku_zuschuss_in_wh => $e[2],
        solarerzeugung_in_wh => $e[3],
        stromverbrauch_in_wh => $e[4],
        solarakku_ladestand_in_promille => $e[5],
        l1_strom_ma => $e[6],
        l2_strom_ma => $e[7],
        l3_strom_ma => $e[8],
        gib_anteil_pv1_in_prozent => $e[9],
        stunden_solarstrahlung => $e[10],
        tages_solarstrahlung => $e[11],
    });
}
close($fh);

print "\nAnzahl der Log-Datensaetze: " . scalar(@$daten) . "\n";
my $logdaten_in_tagen = ($daten->[$#$daten]->{'zeitpunkt'} - $daten->[0]->{'zeitpunkt'}) / 86400;
print "Betrachteter Zeitraum: " . sprintf("%.1f", $logdaten_in_tagen) . " Tage\n";

my $verbrauch = [];
foreach my $e (@$daten) {
    push(@$verbrauch, $e->{stromverbrauch_in_wh});
}
print "Grundverbrauch(via Median): " . (sort(@$verbrauch))[int(scalar(@$verbrauch) / 2)] . " W\n";

my $min_i_in_ma = 0;
my $min_i_phase = 1;
my $max_i_in_ma = 0;
my $max_i_phase = 1;
foreach my $e (@$daten) {
    foreach my $i (1..3) {
        if($e->{"l${i}_strom_ma"} > $max_i_in_ma) {
            $max_i_in_ma = $e->{"l${i}_strom_ma"};
            $max_i_phase = $i;
        }
        if($e->{"l${i}_strom_ma"} < $min_i_in_ma) {
            $min_i_in_ma = $e->{"l${i}_strom_ma"};
            $min_i_phase = $i;
        }
    }
}
print "Min. Strom: " . sprintf("%.2f", $min_i_in_ma / 1000) . " A (Phase $min_i_phase)\n";
print "Max Strom: " . sprintf("%.2f", $max_i_in_ma / 1000) . " A (Phase $max_i_phase)\n";

my $akku_ladezyklen_in_promille = 0;
my $prognostizierte_vollzyklen = 6000;
my $alt = {solarakku_ladestand_in_promille => 0};
foreach my $e (@$daten) {
    my $ladeveraenderung_in_promille = $alt->{solarakku_ladestand_in_promille} - $e->{solarakku_ladestand_in_promille};
    if($ladeveraenderung_in_promille > 0) {
        $akku_ladezyklen_in_promille += $ladeveraenderung_in_promille;
    }
    $alt = $e;
}
print "Vollzyklen des Akkus: " . sprintf("%.2f", $akku_ladezyklen_in_promille / 1000) . "\n";
my $prognose_akku_haltbar_in_tagen = $prognostizierte_vollzyklen / ($akku_ladezyklen_in_promille / 1000) * $logdaten_in_tagen;
print "Haltbarkeit des Akkus(gesamt; bei max. $prognostizierte_vollzyklen Vollzyklen): " . sprintf("%.1f", $prognose_akku_haltbar_in_tagen / 365) . " Jahre\n";

print "\n";
