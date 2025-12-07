#!/usr/bin/perl
use strict;
use warnings;

sub _gib_duchschnitt {
    my ($liste) = @_;
    #return (sort(@$liste))[int(scalar(@$liste) / 2)];
    return 0 if(!scalar(@$liste));

    my $x = 0;
    foreach(@$liste) {
        $x += $_;
    }
    return sprintf("%.4f", $x / scalar(@$liste));
}
sub _hole_daten {
    my $daten = [];
    my @files = ();
    opendir(my $dh, '../sd-karteninhalt') or die $!;
    while(my $filename = readdir($dh)) {
        next if($filename !~ m/^anlage_log\-\d{4}\-\d{2}\.csv$/);

        push(@files, $filename);
    }
    closedir($dh);
    if(!scalar(@files)) {
        die "Bitte erst 'download_der_daten_der_zentrale.pl [IP]' ausfuehren\n";
    }

    my $fehler = 0;
    my $last_error_line = '';
    foreach my $filename (sort(@files)) {
        my $fh;
        if(!open($fh, '<', "../sd-karteninhalt/$filename")) {
            die "Fehler: $!\n";
        }
        $/ = "\n";
        while(my $line = <$fh>) {
            chomp($line);
            my @e = $line =~ m/^(\d{10,}),(e[234]),([\-\d]+),([\-\d]+),(\d+),(\d+),(\d+),([\-\d]+),([\-\d]+),([\-\d]+),(\d+),(\d+),(?:(\d+),|)(?:(\d+),(\d+),(\d+),|)/;
            if(!scalar(@e)) {
                # warn $line;
                $last_error_line = $line;
                $fehler++;
                next;
            }

            my @w = $line =~ m/,w[2],(\d+),(\d+)/;
            if(!scalar(@w)) {
                # warn $line;
                $last_error_line = $line;
                $fehler++;
                next;
            }

            my @t = $line =~ m/,kb?1([\-\d\.]+),([\d\.]+),kl?1([\-\d\.]+),([\d\.]+)(?:,hs(\d),(\d+)|)/;

            my @d = gmtime($e[0]);
            my $neu = {
                zeitpunkt                       => $e[0],
                datum                           => sprintf("%04d-%02d-%02d", $d[5] + 1900, $d[4] + 1, $d[3]),
                stunde                          => $d[2],
                monat                           => $d[4] + 1,
                netzbezug_in_w                  => $e[2],
                solarakku_zuschuss_in_w         => $e[3],
                solarerzeugung_in_w             => $e[4],
                stromverbrauch_in_w             => $e[5],
                solarakku_ladestand_in_promille => $e[6],
                l1_strom_ma                     => $e[7],
                l2_strom_ma                     => $e[8],
                l3_strom_ma                     => $e[9],
                gib_anteil_pv1_in_prozent       => $e[10],
                ersatzstrom_ist_aktiv           => $e[11] ? 1 : 0,
                gesamt_energiemenge_in_wh       => $e[1] eq 'e2' ? 0 : $e[12],

                l1_solarstrom_ma       => ($e[1] eq 'e2' || $e[1] eq 'e3') ? 0 : $e[13],
                l2_solarstrom_ma       => ($e[1] eq 'e2' || $e[1] eq 'e3') ? 0 : $e[14],
                l3_solarstrom_ma       => ($e[1] eq 'e2' || $e[1] eq 'e3') ? 0 : $e[15],

                stunden_solarstrahlung          => $w[0],
                tages_solarstrahlung            => $w[1],

                wohnraum_temperatur                 => $t[0],
                wohnraum_luftfeuchtigkeit           => $t[1],
                bad_temperatur                 => $t[2],
                bad_luftfeuchtigkeit           => $t[3],

                heizungsunterstuetzung_an       => $t[4],
                heizung_temperatur_differenz => $t[5],
            };
            push(@$daten, $neu);
        }
        close($fh);
    }
    if($fehler) {
        warn "ACHTUNG: $fehler Fehler!\n";
        #warn $last_error_line;

    }
    return $daten;
}
my $daten = _hole_daten();
$daten = [sort { $a->{zeitpunkt} <=> $b->{zeitpunkt} } @$daten];
print "\nAnzahl der Log-Datensaetze: " . scalar(@$daten) . "\n";
my $logdaten_in_tagen = ($daten->[$#$daten]->{'zeitpunkt'} - $daten->[0]->{'zeitpunkt'}) / 86400;
print "Betrachteter Zeitraum: " . sprintf("%.1f", $logdaten_in_tagen) . " Tage\n";

my $amp_filename = '3phasiger_lastverlauf.csv';
open(my $amp_file, '>', $amp_filename) or die $!;
print $amp_file "Datum,1,2,3\n";
my ($l1, $l2, $l3) = (0, 0, 0);
my ($l1max, $l2max, $l3max) = (0, 0, 0);
foreach my $e (@$daten) {
    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($e->{'zeitpunkt'});
    #next if($year + 1900 < 2024);
    next if($e->{'zeitpunkt'} < 1754922044); # 11/08/2025 -> wallbox/wp wurde korrekt verkabelt
    $l1 = $e->{"l1_strom_ma"} if($l1 < $e->{"l1_strom_ma"});
    $l2 = $e->{"l2_strom_ma"} if($l2 < $e->{"l2_strom_ma"});
    $l3 = $e->{"l3_strom_ma"} if($l3 < $e->{"l3_strom_ma"});

    $l1max = $l1 if($l1max < $l1);
    $l2max = $l2 if($l2max < $l2);
    $l3max = $l3 if($l3max < $l3);
    if($min == 0) {
        print $amp_file sprintf(
            "%4d-%02d-%02d %d:%02d,%d,%d,%d\n",
            $year + 1900, $mon + 1, $mday, $hour, $min, $e->{"l1_strom_ma"}, $e->{"l2_strom_ma"}, $e->{"l3_strom_ma"}
        );
        ($l1, $l2, $l3) = (0, 0, 0);
    }
}
close($amp_file) or die $!;
print "CSV-Datei $amp_filename wurde erstellt (seit 11.08.2025)\n";
print "L1max: $l1max, L2max: $l2max, L3max: $l3max  (in mA)\n";

foreach my $e (['sommer', [3..9], '800'], ['winter', [10..12,1,2], '1500']) {
    my $name = $e->[0];
    my $monate = $e->[1];
    my $max_grundverbrauch = $e->[2];
    my $verbrauch = [];
    foreach my $e (@$daten) {
        if(
            $e->{stromverbrauch_in_w} < $max_grundverbrauch
            && grep { $_ == $e->{monat} } @$monate
        ) {
            push(@$verbrauch, $e->{stromverbrauch_in_w});
        }
    }
    print "grundverbrauch_in_w_pro_h_$name (via Median): " . (sort(@$verbrauch))[int(scalar(@$verbrauch) / 2)] . " W\n";
}

{
    print "Stromstaerken korrektem Umbau (11.08.2024)\n";
    my $strom = {};
    my $ueberlast_start = 0;
    my $ueberlast_anzahl = 0;
    my $ueberlast_dauer = 0;
    my $ueberlast_max_dauer = 0;
    foreach my $e (@$daten) {
        if($e->{'zeitpunkt'} > 1754922044) { # 11/08/2025 -> wallbox/wp wurde korrekt verkabelt
            if($e->{'netzbezug_in_w'} > 4670) {
                if(!$ueberlast_start) {
                    $ueberlast_start = $e->{'zeitpunkt'};
                    $ueberlast_anzahl++;
                }
            } else {
                if($ueberlast_start) {
                    my $dauer = $e->{'zeitpunkt'} - $ueberlast_start;
                    if($dauer > $ueberlast_max_dauer) {
                        $ueberlast_max_dauer = $dauer;
                    }
                    $ueberlast_dauer += $dauer;
                    $ueberlast_start = 0;
                }
            }

            foreach my $phase (1..3) {
                $strom->{$phase} ||= {
                    min => 0, max => 0,
                    ueberlast_start => 0, ueberlast_anzahl => 0,
                    ueberlast_dauer => 0, ueberlast_max_dauer => 0
                };
                my $i_in_ma = $e->{"l${phase}_strom_ma"};
                if($i_in_ma > 16000) {
                    if(!$strom->{$phase}->{'ueberlast_start'}) {
                        $strom->{$phase}->{'ueberlast_start'} = $e->{'zeitpunkt'};
                        $strom->{$phase}->{'ueberlast_anzahl'}++;
                    }
                } else {
                    if($strom->{$phase}->{'ueberlast_start'}) {
                        my $dauer = $e->{'zeitpunkt'} - $strom->{$phase}->{'ueberlast_start'};
                        if($dauer > $strom->{$phase}->{'ueberlast_max_dauer'}) {
                            $strom->{$phase}->{'ueberlast_max_dauer'} = $dauer;
                        }
                        $strom->{$phase}->{'ueberlast_dauer'} += $dauer;
                        $strom->{$phase}->{'ueberlast_start'} = 0;
                    }
                }
                if($i_in_ma > $strom->{$phase}->{'max'}) {
                    $strom->{$phase}->{'max'} = $i_in_ma;
                }
                if($i_in_ma < $strom->{$phase}->{'min'}) {
                    $strom->{$phase}->{'min'} = $i_in_ma;
                }
            }
        }
    }
    print "Ueberlast: Anzahl = $ueberlast_anzahl, Gesamtdauer = ${ueberlast_dauer}s, max Dauer = ${ueberlast_max_dauer}s\n";
    foreach my $phase (1..3) {
        print "  Phase $phase:\n";
        print "     Min " . sprintf("%.2f", $strom->{$phase}->{'min'} / 1000) . " A\n";
        print "     Max " . sprintf("%.2f", $strom->{$phase}->{'max'} / 1000) . " A\n";
        print "     >16A Anzahl $strom->{$phase}->{'ueberlast_anzahl'}\n";
        print "     >16A Dauer Gesamt $strom->{$phase}->{'ueberlast_dauer'}s\n";
        print "     >16A max.Dauer $strom->{$phase}->{'ueberlast_max_dauer'}\n";
    }
}

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

foreach my $e (['sommer', [3..9]], ['winter', [10..12,1,2]]) {
    my $name = $e->[0];
    my $monate = $e->[1];
    my $strahlungsdaten = {};
    my $zeitraum_daten = {};
    foreach my $monat (@$monate) {
        $strahlungsdaten->{$monat} ||= {};
        my $tmp_strahlungsdaten = $strahlungsdaten->{$monat};
        foreach my $e (@$daten) {
            next if($e->{monat} != $monat);

            next if(!$e->{stunden_solarstrahlung});

            $tmp_strahlungsdaten->{solarerzeugung_in_w} ||= [];
            $tmp_strahlungsdaten->{stunden_solarstrahlung} ||= [];
            push(@{$tmp_strahlungsdaten->{stunden_solarstrahlung}}, $e->{stunden_solarstrahlung});
            push(@{$tmp_strahlungsdaten->{solarerzeugung_in_w}}, $e->{solarerzeugung_in_w});
            push(@{$zeitraum_daten->{stunden_solarstrahlung}}, $e->{stunden_solarstrahlung});
            push(@{$zeitraum_daten->{solarerzeugung_in_w}}, $e->{solarerzeugung_in_w});
        }
    }

    print "Umrechnungsfaktor Solarstrahlung pro h -> Leistung in W (Monatsweise):\n";
    foreach my $monat (@$monate) {
        my $tmp_strahlungsdaten = $strahlungsdaten->{$monat};

        my $stunden_solarstrahlung = _gib_duchschnitt($tmp_strahlungsdaten->{stunden_solarstrahlung});
        next if(!$stunden_solarstrahlung);

        my $erzeugung_in_w = _gib_duchschnitt($tmp_strahlungsdaten->{solarerzeugung_in_w});
        next if(!$erzeugung_in_w);

        my $faktor = $erzeugung_in_w / $stunden_solarstrahlung;
        printf("$monat: %.2f\n", $faktor);
    }
    my $stunden_solarstrahlung = _gib_duchschnitt($zeitraum_daten->{stunden_solarstrahlung});
    my $erzeugung_in_w = _gib_duchschnitt($zeitraum_daten->{solarerzeugung_in_w});
    if($erzeugung_in_w && $stunden_solarstrahlung) {
        my $faktor = $erzeugung_in_w / $stunden_solarstrahlung;
        printf("solarstrahlungs_vorhersage_umrechnungsfaktor_$name: %.2f\n", $faktor * 0.85);
    }
}


# my $min_temp = 999;
# my $min_time = 0;
# my $max_temp = 0;
# my $max_time = 0;
# my $delta_temp = 0;
# my $delta_time = 0;
#
# my $temp_filename = 'temperaturverlauf.csv';
# open(my $temp_file, '>', $temp_filename) or die $!;
# print $temp_file "Datum,Boden,Luft\n";
# foreach my $e (@$daten) {
#     if(
#         $e->{"erde_temperatur"} && $e->{"erde_temperatur"} != 0
#         && $e->{"luft_temperatur"} && $e->{"luft_temperatur"} != 0
#     ) {
#         my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($e->{'zeitpunkt'});
#         if($min % 15 == 0) {
#             print $temp_file sprintf(
#                 "%4d-%02d-%02d %d:%02d,%.1f,%01f\n",
#                 $year + 1900, $mon + 1, $mday, $hour, $min, $e->{erde_temperatur}, $e->{luft_temperatur}
#             );
#         }
#         if($e->{'erde_temperatur'} < $max_temp) {
#             if(
#                 $max_temp
#                 && $min_temp < $max_temp
#                 && sprintf("%.1f", $max_temp - $min_temp) >= 0.4
#             ) {
#                 $delta_temp += $max_temp - $min_temp;
#                 $delta_time += ($max_time - $min_time) / 3600;
#                 # printf(
#                 #     "Delta: %.1f K in %.2f h $min_temp -> $max_temp; %s\n",
#                 #     $max_temp - $min_temp,
#                 #     ($max_time - $min_time) / 3600,
#                 #     '' . localtime($e->{'zeitpunkt'})
#                 # );
#             }
#             $min_temp = $e->{'erde_temperatur'};
#             $min_time = $e->{'zeitpunkt'};
#             $max_temp = $e->{'erde_temperatur'};
#             $max_time = $e->{'zeitpunkt'};
#         } elsif($e->{'erde_temperatur'} > $max_temp) {
#             $max_temp = $e->{'erde_temperatur'};
#             $max_time = $e->{'zeitpunkt'};
#         }
#     }
# }
# close($temp_file) or die $!;
# printf("\nTemperaturertrag: %.2f K/h\n", $delta_temp / $delta_time);
# print "CSV-Datei $temp_filename wurde erstellt\n";

my $wait_till = 0;
my $wday_name = ['So', 'Mo', 'Di', 'Mi', 'Do', 'Fr', 'Sa'];
my $feuchtligkeiten = [];
print "\n\nFeuchtligkeits-Peaks:\n";
foreach my $e (@$daten) {
    next if(!$e->{"bad_luftfeuchtigkeit"});

    push(@$feuchtligkeiten, $e->{"bad_luftfeuchtigkeit"});
    shift(@$feuchtligkeiten) if(scalar(@$feuchtligkeiten) > 10);

    next if($e->{'zeitpunkt'} < 1760948444);

    my $summe = 0;
    map { $summe += $_ } @$feuchtligkeiten;
    my $schnitt = $summe / scalar(@$feuchtligkeiten);

    if(
        $e->{"bad_luftfeuchtigkeit"} >= $schnitt + 5
        &&
        (!$wait_till || $wait_till < $e->{'zeitpunkt'})
    ) {
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($e->{'zeitpunkt'});
        print sprintf(
            "\"%4d-%02d-%02d %d:%02d\",\n",
            $year + 1900, $mon + 1, $mday, $hour, $min
        );
        # print sprintf(
        #     "%s %4d-%02d-%02d %d:%02d,%.1f,%.1f\n",
        #     $wday_name->[$wday], $year + 1900, $mon + 1, $mday, $hour, $min, $schnitt, $e->{bad_luftfeuchtigkeit}
        # );
        $wait_till = $e->{'zeitpunkt'} + 3600;
    }
}

my $energiemenge = {};
print "\nErzeugte Energiemenge (Monatsweise):\n";
my $gesamt_energiemenge_in_wh_startpunkt = 0;
foreach my $e (@$daten) {
    next if(!$e->{'gesamt_energiemenge_in_wh'} || $e->{'zeitpunkt'} < 1736857686);# 14.1.2025

    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($e->{'zeitpunkt'});
    $mon += 1;
    $year += 1900;
    my $key = sprintf("%02d/%04d", $mon, $year);
    if(!defined($energiemenge->{$key})) {
        $gesamt_energiemenge_in_wh_startpunkt = $e->{'gesamt_energiemenge_in_wh'};
    }
    $energiemenge->{$key} = $e->{'gesamt_energiemenge_in_wh'} - $gesamt_energiemenge_in_wh_startpunkt;
}
my $erster_monat = 1;
foreach my $key (sort(keys(%$energiemenge))) {
    printf("%s: %.0f kWh %s\n", $key, $energiemenge->{$key} / 1000, ($erster_monat ? ' (nur teilweise!)' : ''));
    $erster_monat = 0;
}

# print "Daten der Heizungs-Temperatur-Differenz\n";
# foreach my $e (@$daten) {
#     next if(!$e->{'heizung_temperatur_differenz'} || $e->{'zeitpunkt'} < "1740222177");
#
#     my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($e->{'zeitpunkt'});
#     my $time = sprintf("%04d-%02d-%02d %02d:%02d:%02d", $year + 1900, $mon + 1, $mday, $hour, $min, $sec);
#     print "$time  $e->{'heizung_temperatur_differenz'}\n";
# }

my $heizstab_an_zeitpunkt = 0;
my $heizstab_tage = {};
my $heizstab_jahre = {};
foreach my $e (@$daten) {
    next if(!exists($e->{'heizungsunterstuetzung_an'}));

    if(
        $e->{'heizungsunterstuetzung_an'}
        && !$heizstab_an_zeitpunkt
        && $e->{'l2_strom_ma'} > 6
    ) {
        $heizstab_an_zeitpunkt = $e->{'zeitpunkt'};
    } elsif($heizstab_an_zeitpunkt > 0) {
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime($e->{'zeitpunkt'});
        if($mon + 1 > 5 && $mon + 1 < 10) {# Da wird nicht geheizt, laesst sich leider nicht anders ermitteln
            $heizstab_an_zeitpunkt = 0;
            next;
        }
        my $day = sprintf("%04d-%02d-%02d", $year + 1900, $mon + 1, $mday);
        my $zeitraum_der_aktvierung = $e->{'zeitpunkt'} - $heizstab_an_zeitpunkt;
        $heizstab_tage->{$day} += $zeitraum_der_aktvierung;
        $heizstab_jahre->{$year + 1900} += $zeitraum_der_aktvierung;
        $heizstab_an_zeitpunkt = 0;
    }
}
print "\n\nDaten der Heizstab Nutzungsdauer pro Tag\n";
foreach my $day (sort keys(%$heizstab_tage)) {
    my $nutzung = 0;
    my $kwh = 0;
    my $kosten = 0;
    if($heizstab_tage->{$day} > 0) {
        $nutzung = $heizstab_tage->{$day} / 86400 * 100;
        $kwh = $heizstab_tage->{$day} / 3600 * 1.5;
        $kosten = $kwh * 0.33;
    }
    printf("%s: %2d %%, %2.1f kWh, %2.2f EUR\n", $day, $nutzung, $kwh, $kosten);
}
print "\n\nDaten der Heizstab Nutzungsdauer pro Jahr\n";
foreach my $year (sort keys(%$heizstab_jahre)) {
    my $kwh = 0;
    my $kosten = 0;
    if($heizstab_jahre->{$year} > 0) {
        $kwh = $heizstab_jahre->{$year} / 3600 * 1.5;
        $kosten = $kwh * 0.33;
    }
    printf("%s: %2.1f kWh, %2.2f EUR\n", $year, $kwh, $kosten);
}

print "\n";
