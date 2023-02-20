#!/usr/bin/perl
use strict;
use warnings;

sub _hole_daten {
    my $daten = [];
    open(my $fh, '<', './anlagen_log.csv') or die "Bitte erst 'lade_aktuelle_logdatei.pl [ESP-IP]' ausfuehren\n";
    $/ = "\n";
    my $alt = {};
    while(my $line = <$fh>) {
        chomp($line);

        my @e = $line =~ m/^(\d+),e1,([\-\d]+),([\-\d]+),(\d+),(\d+),(\d+),([\-\d]+),([\-\d]+),([\-\d]+),(\d+),w1,(\d+),(\d+)$/;
        next if(!scalar(@e));

        my @d = gmtime($e[0]);
        my $neu = {
            zeitpunkt => $e[0],
            datum => ($d[5] + 1900) . '-' . ($d[4] + 1) . '-' . $d[3],
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
        if(exists($alt->{zeitpunkt})) {
            my $zeit_in_h = ($neu->{zeitpunkt} - $alt->{zeitpunkt}) / 3600;
            $neu->{solarenergie_in_wh} = $neu->{solarerzeugung_in_w} * $zeit_in_h;
        } else {
            $neu->{solarenergie_in_wh} = 0;
        }
        push(@$daten, $neu);
        $alt = $neu;
    }
    close($fh);
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

# my $stundenstrahlung_verhaeltnisse = [];
my $daten_nach_stunden = {};
foreach my $e (@$daten) {
    # TODO Die TAge so auftrennen, dass pro Stunde eineListe entsteht, aus der die MEridiane gebildet werden koennens
    my $key = $e->{datum} . ':' . $e->{stunde};
    $daten_nach_stunden->{$key} ||= {
        solarenergie_in_wh     => 0,
        stahlungs_vorhersage => 0,
    };
    $daten_nach_stunden->{$key}->{stahlungs_vorhersage} = $e->{stunden_solarstrahlung};
    $daten_nach_stunden->{$key}->{solarenergie_in_wh} += $e->{solarenergie_in_wh};
    # TODO jeden Tag grupieren, dann median bilden, bei beiden werten.
}
use Data::Dumper;
die Dumper($daten_nach_stunden);
# foreach my $e (@{$daten_nach_stunden->{12}}) {
#     print $e->{solarenergie_in_wh}."\n";
# }
#     # solarerzeugung_in_w
#     #stunden_solarstrahlung
#     #tages_solarstrahlung
#     push(
#         @$stundenstrahlung_verhaeltnisse,
#         ($e->{solarerzeugung_in_w} > 0 ? $e->{stunden_solarstrahlung} * 1000 / $e->{solarerzeugung_in_w} : 0)
#     );
#     warn "$e->{stunden_solarstrahlung} / $e->{solarerzeugung_in_w} = " . ($e->{stunden_solarstrahlung} * 1000 / $e->{solarerzeugung_in_w});
# }
# print "Stundenvorhersage 1 W Strahlung ergibt (via Median): " . (sort(@$stundenstrahlung_verhaeltnisse))[int(scalar(@$stundenstrahlung_verhaeltnisse) / 2)] . " W Strom\n";

print "\n";
