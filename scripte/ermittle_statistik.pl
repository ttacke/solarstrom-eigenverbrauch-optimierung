#!/usr/bin/perl
use strict;
use warnings;

sub _hole_daten {
    my $daten = [];
    foreach my $filename (qw/
        anlage_log.csv.2023-03-03
        anlage_log.csv
    /) {
        my $fh;
        if(!open($fh, '<', "../sd-karteninhalt/$filename")) {
            if($filename eq 'anlage_log.csv') {
                die "Bitte erst 'download_der_daten_der_zentrale.pl [IP]' ausfuehren\n";
            } else {
                next;
            }
        }
        $/ = "\n";
        my $alt = {};

        while(my $line = <$fh>) {
            chomp($line);

            my @e = $line =~ m/^(\d+),e1,([\-\d]+),([\-\d]+),(\d+),(\d+),(\d+),([\-\d]+),([\-\d]+),([\-\d]+),(\d+),w[12],(\d+),(\d+)/;
            next if(!scalar(@e));

            my @d = gmtime($e[0]);
            my $neu = {
                zeitpunkt => $e[0],
                datum => sprintf("%04d-%02d-%02d", $d[5] + 1900, $d[4] + 1, $d[3]),
                stunde => $d[2],
                monat => $d[4] + 1,
                netzbezug_in_w => $e[1],
                solarakku_zuschuss_in_w => $e[2],
                solarerzeugung_in_w => $e[3],
                stromverbrauch_in_w => $e[4],
                solarakku_ladestand_in_promille => $e[5],
                l1_strom_ma => $e[6],
                l2_strom_ma => $e[7],
                l3_strom_ma => $e[8],
                gib_anteil_pv1_in_prozent => $e[9],
                stunden_solarstrahlung => $e[10],
                tages_solarstrahlung => $e[11],
            };
            push(@$daten, $neu);
            $alt = $neu;
        }
        close($fh);
    }
    return $daten;
}
my $daten = _hole_daten();

print "\nAnzahl der Log-Datensaetze: " . scalar(@$daten) . "\n";
my $logdaten_in_tagen = ($daten->[$#$daten]->{'zeitpunkt'} - $daten->[0]->{'zeitpunkt'}) / 86400;
print "Betrachteter Zeitraum: " . sprintf("%.1f", $logdaten_in_tagen) . " Tage\n";

my $verbrauch = [];
foreach my $e (@$daten) {
    push(@$verbrauch, $e->{stromverbrauch_in_w});
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
print "Max. Strom: " . sprintf("%.2f", $max_i_in_ma / 1000) . " A (Phase $max_i_phase)\n";

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
# TODO 20-80% ist weniger schlimm, 40-60 am wenigsten. Das hier mit einem zusätzlichen Wert angeben

my $tmp_strahlungsdaten = {};
foreach my $e (@$daten) {
    my $key = $e->{datum} . ':' . $e->{stunde};
    $tmp_strahlungsdaten->{$key} ||= {
        solarerzeugung_in_w   => [],
        stunden_solarstrahlung => $e->{stunden_solarstrahlung},
        leistung_in_w        => 0,
        stunde               => $e->{stunde},
        monat                => $e->{monat},
    };
    push(@{$tmp_strahlungsdaten->{$key}->{solarerzeugung_in_w}}, $e->{solarerzeugung_in_w});
}

my $vorhersage_verhaeltnisse = {};
foreach my $key (keys(%$tmp_strahlungsdaten)) {
    my $e = $tmp_strahlungsdaten->{$key};
    next if(!$e->{stunden_solarstrahlung});

    my $erzeugung_in_w_median = (sort(@{$e->{solarerzeugung_in_w}}))[int(scalar(@{$e->{solarerzeugung_in_w}}) / 2)];
    next if(!$erzeugung_in_w_median);

    $vorhersage_verhaeltnisse->{$e->{monat}} ||= {};
    $vorhersage_verhaeltnisse->{$e->{monat}}->{$e->{stunde}} ||= [];
    my $liste = $vorhersage_verhaeltnisse->{$e->{monat}}->{$e->{stunde}};
    print "int($erzeugung_in_w_median * 1000 / $e->{stunden_solarstrahlung})\n";
    print int($erzeugung_in_w_median * 1000 / $e->{stunden_solarstrahlung}) . "\n";
    push(@$liste, int($erzeugung_in_w_median * 1000 / $e->{stunden_solarstrahlung}));

# TODO ist nicht linear? GGf besser nicht in Stunden, sondern in
#StrahlungsBlöcke teilen? ISt 0-100 ein halbwegs gleichbleibender Wert?
#und 100-200? 200-300 etc?
# Hohe Strahlungswerte haben sehr hohe leistungen, kleine Werte aber sehr kleine Leistungen
}

print "Ist-Leistung in kW / Vorhersage in w/m2:\n";
foreach my $monat (1..12) {
    my $line = sprintf(" %02d: ", $monat);
    foreach my $stunde (0..23) {
        my $liste = $vorhersage_verhaeltnisse->{$monat}->{$stunde};
        my $median_verhaeltnis = '-';
        if($liste) {
            $median_verhaeltnis = (sort(@{$liste}))[int(scalar(@{$liste}) / 2)];
        }
        $line .= sprintf("%02d: %s ", $stunde, $median_verhaeltnis);
    }
    print "$line\n";
}

print "\n";
