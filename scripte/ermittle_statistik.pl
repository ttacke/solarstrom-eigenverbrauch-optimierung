#!/usr/bin/perl
use strict;
use warnings;
$| = 1;

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
    print "Hole Daten...\n";
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
    my $i = 0;
    foreach my $filename (sort(@files)) {
        my $fh;
        if(!open($fh, '<', "../sd-karteninhalt/$filename")) {
            die "Fehler: $!\n";
        }
        $/ = "\n";
        while(my $line = <$fh>) {
            $i++;
            printf("\r%3.2f Mio zeilen", $i / 1000000) if($i % 10000 == 0);
            chomp($line);
            if($line =~ /\.\./ || $line !~ /^\d{10}/) {
                # warn $line;
                $last_error_line = $line;
                $fehler++;
                next;
            }
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

            # my @auto = $line =~ m/,va[4],([a-z]+),([a-z]+),([a-z]+),([\d\.]+),([\d\.]+),([\d\.]+),([a-z]+),([a-z]+)/;
            # my @roller = $line =~ m/,vb[8],([a-z]+),([a-z]+),([a-z]+),([\d\.]+),([\d\.]+),([\d\.]+),([a-z]+),([a-z]+),([a-z]+)/;
            my @luftheiz_und_begleitheiz = $line =~ m/,vc[1],([a-z]+),([\d]+),([a-z]+),([\d]+)/;

            my @d = gmtime($e[0]);
            my $neu = {
                zeitpunkt                       => $e[0],
                datum                           => sprintf("%04d-%02d-%02d", $d[5] + 1900, $d[4] + 1, $d[3]),
                minute => sprintf('%02d', $d[1]),
                stunde => sprintf('%02d', $d[2]),
                tag => sprintf('%02d', $d[3]),
                monat => sprintf('%02d', $d[4] + 1),
                jahr => sprintf('%02d', $d[5] + 1900),

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

                waermepumpen_zuluft_temperatur                 => $t[0],
                waermepumpen_zuluft_luftfeuchtigkeit           => $t[1],
                waermepumpen_abluft_temperatur                 => $t[2],
                waermepumpen_abluft_luftfeuchtigkeit           => $t[3],

                heizungsunterstuetzung_an       => $t[4],
                heizung_temperatur_differenz => $t[5],
            };
            if(scalar(@luftheiz_und_begleitheiz)) {
                $neu->{'heiz_luftvorwaermer_an'} = ($luftheiz_und_begleitheiz[0] eq 'on' ? 1 : 0);
                $neu->{'heiz_luftvorwaermer_leistung'} = $luftheiz_und_begleitheiz[1];

                $neu->{'begleitheizung_an'} = ($luftheiz_und_begleitheiz[2] eq 'on' ? 1 : 0);
                $neu->{'begleitheizung_leistung'} = $luftheiz_und_begleitheiz[3];
            };
            push(@$daten, $neu);
        }
        close($fh);
    }
    printf("\r%3.2f Mio zeilen\n", $i / 1000000);
    print "Sortiere...\n";
    $daten = [sort { $a->{zeitpunkt} <=> $b->{zeitpunkt} } @$daten];
    print "...Fertig\n";
    if($fehler) {
        warn "ACHTUNG: $fehler Fehler!\n";
        #warn $last_error_line;

    }
    return $daten;
}
my $daten = _hole_daten();
print "\nAnzahl der Log-Datensaetze: " . scalar(@$daten) . "\n";
my $logdaten_in_tagen = ($daten->[$#$daten]->{'zeitpunkt'} - $daten->[0]->{'zeitpunkt'}) / 86400;
print "Betrachteter Zeitraum: " . sprintf("%.1f", $logdaten_in_tagen) . " Tage\n";
{
    my $amp_filename = '3phasiger_lastverlauf.csv';
    my $amp_detail_filename = '3phasiger_lastverlauf_detail.csv';
    open(my $amp_file, '>', $amp_filename) or die $!;
    open(my $amp_detail_file, '>', $amp_detail_filename) or die $!;
    print $amp_file "Datum,L1,L2,L3,WPan\n";
    print $amp_detail_file "Datum,L1,L2,L3,WPan,Abluft,Zuluft,LuftHeizAn,LuftHeiz\n";
    my ($l1, $l2, $l3) = (0, 0, 0);
    my ($l1max, $l2max, $l3max) = (0, 0, 0);
    my $wp_an = 0;
    my $abluft_letzter_wert = 0;
    my $wp_an_letzter_wert = 0;
    my $letzter_log_zeitpunkt = $daten->[$#$daten]->{'zeitpunkt'};
    foreach my $e (@$daten) {
        #next if($e->{'zeitpunkt'} < 1754922044); # 11/08/2025 -> wallbox/wp wurde korrekt verkabelt
        next if($e->{'zeitpunkt'} < 1766962800); # 29/12/2025 -> wp zu/abluft korrekt vermessen

        $l1 = $e->{"l1_strom_ma"} if($l1 < $e->{"l1_strom_ma"});
        $l2 = $e->{"l2_strom_ma"} if($l2 < $e->{"l2_strom_ma"});
        $l3 = $e->{"l3_strom_ma"} if($l3 < $e->{"l3_strom_ma"});

        $l1max = $e->{"l1_strom_ma"} if($l1max < $e->{"l1_strom_ma"});
        $l2max = $e->{"l2_strom_ma"} if($l2max < $e->{"l2_strom_ma"});
        $l3max = $e->{"l3_strom_ma"} if($l3max < $e->{"l3_strom_ma"});
        if(
            $wp_an
            && $abluft_letzter_wert > 0
            && $e->{'waermepumpen_abluft_temperatur'} > 0
            && (
                $abluft_letzter_wert - $e->{'waermepumpen_abluft_temperatur'} <= -2.0
            )
        ) {
            $wp_an = 0;
        } elsif(
            !$wp_an
            && $abluft_letzter_wert > 0
            && $e->{'waermepumpen_abluft_temperatur'} > 0
            && (
                $abluft_letzter_wert - $e->{'waermepumpen_abluft_temperatur'} >= 2.0
            )
        ) {
            $wp_an = 1;
        }
        $e->{'_heizung_waermepumpe_status'} = $wp_an;
        if($e->{'zeitpunkt'} > $letzter_log_zeitpunkt - 86400 * 2) {
            print $amp_detail_file sprintf(
                "%4d-%02d-%02d %d:%02d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                $e->{'jahr'}, $e->{'monat'}, $e->{'tag'}, $e->{'stunde'}, $e->{'minute'},
                $e->{"l1_strom_ma"}, $e->{"l2_strom_ma"}, $e->{"l3_strom_ma"}, $wp_an*5000,
                $e->{'waermepumpen_abluft_temperatur'} * 1000,
                $e->{'waermepumpen_zuluft_temperatur'} * 1000,
                $e->{'heiz_luftvorwaermer_an'} * 6000,
                $e->{'heiz_luftvorwaermer_leistung'}
            );
        }
        if($e->{'minute'} == 0 || $wp_an_letzter_wert != $wp_an) {
            print $amp_file sprintf(
                "%4d-%02d-%02d %d:%02d,%d,%d,%d,%d\n",
                $e->{'jahr'}, $e->{'monat'}, $e->{'tag'}, $e->{'stunde'}, $e->{'minute'},
                $l1, $l2, $l3, $wp_an*5000
            );
            ($l1, $l2, $l3) = (0, 0, 0);
        }
        $wp_an_letzter_wert = $wp_an;
        if($e->{'waermepumpen_abluft_temperatur'} > 0) {
            $abluft_letzter_wert = $e->{'waermepumpen_abluft_temperatur'};
        }
    }
    close($amp_file) or die $!;
    close($amp_detail_file) or die $!;
    print "CSV-Datei $amp_filename (seit 11.08.2025) + $amp_detail_filename (letzte 24h) wurde erstellt\n";
    print "L1max: $l1max, L2max: $l2max, L3max: $l3max  (in mA)\n";
}
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
    my $maximal_zulaessige_last_in_w = 9000;# ((59kVA (Syna) / 12) + 4.6kVA (lasterhoehung)) * 0,95 (VDE-AR-N 4100) = 9000 W
    my $maximale_leistung_pro_phase_in_ma = 24000;# eigentlich 32000 Haussicherung, aber wir wollen ja keinen Stress
    foreach my $e (@$daten) {
        if($e->{'zeitpunkt'} > 1754922044) { # 11/08/2025 -> wallbox/wp wurde korrekt verkabelt
            if($e->{'netzbezug_in_w'} > $maximal_zulaessige_last_in_w) {
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
                if($i_in_ma > $maximale_leistung_pro_phase_in_ma) {
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
    print "Ueberlast(> $maximal_zulaessige_last_in_w W): Anzahl = $ueberlast_anzahl, Gesamtdauer = ${ueberlast_dauer}s, max Dauer = ${ueberlast_max_dauer}s\n";
    foreach my $phase (1..3) {
        print "  Phase $phase:\n";
        print "     Min " . sprintf("%.2f", $strom->{$phase}->{'min'} / 1000) . " A\n";
        print "     Max " . sprintf("%.2f", $strom->{$phase}->{'max'} / 1000) . " A\n";
        print "     >$maximale_leistung_pro_phase_in_ma mA Anzahl $strom->{$phase}->{'ueberlast_anzahl'}\n";
        print "     >$maximale_leistung_pro_phase_in_ma mA Dauer Gesamt $strom->{$phase}->{'ueberlast_dauer'}s\n";
        print "     >$maximale_leistung_pro_phase_in_ma mA max.Dauer $strom->{$phase}->{'ueberlast_max_dauer'}\n";
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
{
    my $energiemenge = {};
    print "\nErzeugte Energiemenge (Monatsweise):\n";
    my $gesamt_energiemenge_in_wh_startpunkt = 0;
    my $netzbezug = 0;
    my $einspeisung = 0;
    my $letzter_zeitpunkt = 0;
    my $letzter_key = 0;
    my $letzter_zeitpunkt_der_daten = $daten->[$#$daten]->{'zeitpunkt'};
    foreach my $e (@$daten) {
        next if(!$e->{'gesamt_energiemenge_in_wh'} || $e->{'zeitpunkt'} < 1736857686);# 14.1.2025

        my $key = sprintf("%02d/%04d", $e->{'monat'}, $e->{'jahr'});
        if($letzter_zeitpunkt) {
            my $veranderung = $e->{'netzbezug_in_w'} / 3600 * ($e->{'zeitpunkt'} - $letzter_zeitpunkt);
            if($veranderung > 0) {
                $netzbezug += $veranderung;
            } else {
                $einspeisung += $veranderung;
            }
        }
        if(
            ($letzter_key && $letzter_key ne $key)
            || $e->{'zeitpunkt'} == $letzter_zeitpunkt_der_daten
        ) {
            $energiemenge->{$letzter_key} = [
                $e->{'gesamt_energiemenge_in_wh'} - $gesamt_energiemenge_in_wh_startpunkt,
                $netzbezug,
                $einspeisung
            ];
        }
        if(!defined($energiemenge->{$key})) {
            $gesamt_energiemenge_in_wh_startpunkt = $e->{'gesamt_energiemenge_in_wh'};
            $netzbezug = 0;
            $einspeisung = 0;
            $energiemenge->{$key} = [0, 0, 0];
        }
        $letzter_key = $key;
        $letzter_zeitpunkt = $e->{'zeitpunkt'};
    }
    my $erster_monat = 1;
    foreach my $key (sort(keys(%$energiemenge))) {
        printf(
            "%s: erzeugt %5.0f kWh, netzbezug %5.0f kWh, einspeisung %5.0f kWh %s\n",
            $key,
            $energiemenge->{$key}->[0] / 1000,
            $energiemenge->{$key}->[1] / 1000,
            $energiemenge->{$key}->[2] / 1000,
            ($erster_monat ? ' (nur teilweise!)' : '')
        );
        $erster_monat = 0;
    }
}
{
    my $heizstab_an_zeitpunkt = 0;
    my $wp_an_zeitpunkt = 0;
    my $wp_laufzeit = 0;
    my $wp_taktung = 0;
    my $heizstab_tage = {};
    my $heizstab_monate = {};
    my $heizstab_war_vermutlich_an = 0;
    my $haus_lief_mit_solar = 0;
    my $last_day = 0;
    my $last_month = 0;
    my $last_timestamp = $daten->[$#$daten]->{'zeitpunkt'};
    foreach my $e (@$daten) {
        next if($e->{'zeitpunkt'} < 1754922044); # 11/08/2025 -> wallbox/wp wurde korrekt verkabelt

        next if($e->{'monat'} > 5 && $e->{'monat'} < 10);# Da wird nicht geheizt, laesst sich leider nicht anders ermitteln

        my $day = sprintf("%04d-%02d-%02d", $e->{'jahr'}, $e->{'monat'}, $e->{'tag'});
        my $month = sprintf("%04d-%02d", $e->{'jahr'}, $e->{'monat'});

        $heizstab_tage->{$day} ||= [0, 0, 0, 0, -1];
        $heizstab_monate->{$month} ||= [0, 0, 0, 0, 0];

        if(
            $wp_an_zeitpunkt > 0
            && (
                $e->{'_heizung_waermepumpe_status'} == 0
                || $last_timestamp == $e->{'zeitpunkt'}
                || ($last_day && $last_day ne $day)
            )
        ) {
            $wp_laufzeit += $e->{'zeitpunkt'} - $wp_an_zeitpunkt;
            $wp_an_zeitpunkt = 0;
        }

        if(
            $last_day && $last_day ne $day
            || $last_timestamp == $e->{'zeitpunkt'}
        ) {
            $heizstab_tage->{$last_day}->[2] = $wp_laufzeit;
            $heizstab_monate->{$last_month}->[2] += $wp_laufzeit;
            $heizstab_tage->{$last_day}->[3] = $wp_taktung;
            $heizstab_monate->{$last_month}->[3] += $wp_taktung;
            $wp_laufzeit = 0;
            $wp_an_zeitpunkt = 0;
            $wp_taktung = 0;
        }

        if($heizstab_an_zeitpunkt > 0 && !$heizstab_war_vermutlich_an && $e->{'l2_strom_ma'} > 6) {
            $heizstab_war_vermutlich_an = 1;
        }
        if(
            $e->{'solarerzeugung_in_w'} > 1500
            && $e->{'solarakku_ladestand_in_promille'} > 400
        ) {
            $haus_lief_mit_solar = 1;
        }
        if(
            exists($e->{'_heizung_waermepumpe_status'})
            && $e->{'_heizung_waermepumpe_status'} == 1
            && (
                !$wp_an_zeitpunkt
                || ($last_day && $last_day ne $day)
            )
        ) {
            $wp_an_zeitpunkt = $e->{'zeitpunkt'};
            $wp_taktung++;
        }

        if(
            exists($e->{'heiz_luftvorwaermer_leistung'})
            && $e->{'heiz_luftvorwaermer_leistung'} > 0
        ) {
            my $verbrauch = $e->{'heiz_luftvorwaermer_leistung'} / (1000 * 60); # 1min bei 0,85 kw
            $heizstab_tage->{$day}->[4] = 0 if($heizstab_tage->{$day}->[4] < 0);
            $heizstab_tage->{$day}->[4] += $verbrauch;
            $heizstab_monate->{$month}->[4] += $verbrauch;
        }

        if(exists($e->{'heizungsunterstuetzung_an'})) {
            if(
                $e->{'heizungsunterstuetzung_an'}
                && !$heizstab_an_zeitpunkt
            ) {
                $heizstab_an_zeitpunkt = $e->{'zeitpunkt'};
                $heizstab_war_vermutlich_an = 0;
                $haus_lief_mit_solar = 0;
            } elsif($heizstab_an_zeitpunkt > 0 && !$e->{'heizungsunterstuetzung_an'}) {
                if($heizstab_war_vermutlich_an == 1) {
                    my $zeitraum_der_aktivierung = $e->{'zeitpunkt'} - $heizstab_an_zeitpunkt;
                    my $index = 0;
                    if(
                        $haus_lief_mit_solar == 1
                    ) {
                        $index = 1;
                    }
                    $heizstab_tage->{$day}->[$index] += $zeitraum_der_aktivierung;
                    $heizstab_monate->{$month}->[$index] += $zeitraum_der_aktivierung;
                }
                $heizstab_an_zeitpunkt = 0;
            }
        }
        $last_day = $day;
        $last_month = $month;
    }

    print "\n\nHeizungsdaten der letzte 14 Tage\n";
    print "           |  Heizstab Netz  |  Heizstab Solar |      Waermepumpe     | Luftwaermer |    Gesamt\n";
    print "       Tag |  %   kWh    EUR |  %   kWh    EUR |  %   kWh    EUR Takt |  kWh    EUR |  kWh    EUR\n";
    my @heizstab_tage = sort keys(%$heizstab_tage);
    my $strom_kosten = 0.33; # EUR
    my $heizstab_leistung = 1.5;
    my $wp_leistung = 0.5;
    foreach my $day (@heizstab_tage[-14..-1]) {
        my $nutzung = 0;
        my $kwh = 0;
        my $kosten = 0;
        if($heizstab_tage->{$day}->[0] > 0) {
            $nutzung = $heizstab_tage->{$day}->[0] / 86400 * 100;
            $kwh = $heizstab_tage->{$day}->[0] / 3600 * $heizstab_leistung;
            $kosten = $kwh * $strom_kosten;
        }
        my $nutzung_solar = 0;
        my $kwh_solar = 0;
        my $kosten_solar = 0;
        if($heizstab_tage->{$day}[1] > 0) {
            $nutzung_solar = $heizstab_tage->{$day}->[1] / 86400 * 100;
            $kwh_solar = $heizstab_tage->{$day}->[1] / 3600 * $heizstab_leistung;
            $kosten_solar = $kwh_solar * $strom_kosten;
        }
        my $wp_kwh = $heizstab_tage->{$day}->[2] / 3600 * $wp_leistung;
        my $wp_kosten = $wp_kwh * $strom_kosten;
        my $luftwaermer_kwh = $heizstab_tage->{$day}->[4] > 0 ? $heizstab_tage->{$day}->[4] : 0;
        my $gesamt_kwh = $kwh + $kwh_solar + $wp_kwh + $luftwaermer_kwh;
        my $gesamt_kosten = ($gesamt_kwh - $kwh_solar) * $strom_kosten;
        printf(
            "%s | %2d  %4.1f  %5.2f | %2d  %4.1f  %5.2f | %2d  %4.1f  %5.2f   %2d | %4.1f  %5.2f | %4.1f  %5.2f\n",
            $day,
            $nutzung, $kwh, $kosten,
            $nutzung_solar, $kwh_solar, $kosten_solar,
            ($heizstab_tage->{$day}->[2] / 86400 * 100), $wp_kwh, $wp_kosten, $heizstab_tage->{$day}->[3],
            $luftwaermer_kwh, $luftwaermer_kwh * $strom_kosten,
            $gesamt_kwh, $gesamt_kosten
        );
    }
    print "\n\nHeizungsdaten pro Monat\n";
    print "        | HeizstabNetz | HeizstbSolar |    Waermepumpe    |  Luftwaermer |     Gesamt\n";
    print "  Monat |   kWh    EUR |   kWh    EUR |   kWh    EUR Takt |   kWh    EUR |   kWh     EUR\n";
    foreach my $month (sort keys(%$heizstab_monate)) {
        my $kwh = 0;
        my $kosten = 0;
        if($heizstab_monate->{$month}->[0] > 0) {
            $kwh = $heizstab_monate->{$month}->[0] / 3600 * $heizstab_leistung;
            $kosten = $kwh * $strom_kosten;
        }
        my $kwh_solar = 0;
        my $kosten_solar = 0;
        if($heizstab_monate->{$month}->[1] > 0) {
            $kwh_solar = $heizstab_monate->{$month}->[1] / 3600 * $heizstab_leistung;
            $kosten_solar = $kwh_solar * $strom_kosten;
        }
        my $wp_kwh = $heizstab_monate->{$month}->[2] / 3600 * $wp_leistung;
        my $luft_kwh = $heizstab_monate->{$month}->[4];
        my $kwh_gesamt = $kwh + $kwh_solar + $wp_kwh + $luft_kwh;
        my $kosten_gesamt = ($kwh_gesamt - $kwh_solar) * $strom_kosten;
        printf(
            "%s | %5.1f  %5.2f | %5.1f  %5.2f | %5.1f  %5.2f   %2d | %5.1f  %5.2f | %5.1f  %6.2f\n",
            $month,
            $kwh, $kosten,
            $kwh_solar, $kosten_solar,
            $wp_kwh, $wp_kwh * $strom_kosten, $heizstab_monate->{$month}->[3],
            $luft_kwh, $luft_kwh * $strom_kosten,
            $kwh_gesamt, $kosten_gesamt
        );
    }
}
print "\n";
